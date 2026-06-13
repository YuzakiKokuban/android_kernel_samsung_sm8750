#include "sec_input.h"

__visible_for_testing int sec_input_device_init(struct device *dev, enum input_device_type type, void *data);
__visible_for_testing void sec_input_set_prop(struct device *dev, struct input_dev *input_dev, enum input_device_type type, void *data);
__visible_for_testing void sec_input_set_prop_touch(struct device *dev, struct input_dev *input_dev, u8 propbit, void *data);
__visible_for_testing void sec_input_set_prop_proximity(struct device *dev, struct input_dev *input_dev, void *data);

int sec_input_device_register(struct device *dev, void *data)
{
	int ret = 0;
	enum input_device_type i;

	/* register input_dev */
	for (i = INPUT_DEVICE_TYPE_TOUCH; i < INPUT_DEVICE_TYPE_MAX; i++) {
		ret = sec_input_device_init(dev, i, data);
		if (ret) {
			input_err(true, dev, "%s: Unable to register Input_Device_type:%d\n",
					__func__, i);
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(sec_input_device_register);


__visible_for_testing int sec_input_device_init(struct device *dev, enum input_device_type type, void *data)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct input_dev *input_dev = NULL;

	switch (type) {
	case INPUT_DEVICE_TYPE_TOUCH:
		input_dev = efunc.devm_input_allocate_device(dev);
		if (!input_dev) {
			input_err(true, dev, "%s: allocate input_dev err!\n", __func__);
			return -ENOMEM;
		}

		if (GET_DEV_COUNT(pdata->multi_dev) == MULTI_DEV_SUB)
			input_dev->name = "sec_touchscreen2";
		else
			input_dev->name = "sec_touchscreen";
		pdata->input_dev = input_dev;
		break;
	case INPUT_DEVICE_TYPE_TOUCHPAD:
		if (pdata->support_dex) {
			input_dev = efunc.devm_input_allocate_device(dev);
			if (!input_dev) {
				input_err(true, dev, "%s: allocate input_dev err!\n", __func__);
				return -ENOMEM;
			}

			input_dev->name = "sec_touchpad";
			pdata->input_dev_pad = input_dev;
		} else {
			input_err(true, dev, "%s: Don't support dex!\n", __func__);
			return SEC_SUCCESS;
		}
		break;
	case INPUT_DEVICE_TYPE_PROXIMITY:
		if (pdata->support_ear_detect || pdata->support_lightsensor_detect) {
			input_dev = efunc.devm_input_allocate_device(dev);
			if (!input_dev) {
				input_err(true, dev, "%s: allocate input_dev err!\n", __func__);
				return -ENOMEM;
			}

			if (GET_DEV_COUNT(pdata->multi_dev) == MULTI_DEV_SUB)
				input_dev->name = "sec_touchproximity2";
			else
				input_dev->name = "sec_touchproximity";
			pdata->input_dev_proximity = input_dev;
		} else {
			input_err(true, dev, "%s: Don't support ear_detect and support lightsensor_detect!\n", __func__);
			return SEC_SUCCESS;
		}
		break;
	default:
		input_err(true, dev, "%s: Don't apply input_device_type! %d\n", __func__, type);
		return SEC_ERROR;
	}

	sec_input_set_prop(dev, input_dev, type, data);
	return efunc.input_register_device(input_dev);
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_device_init);
#endif

__visible_for_testing void sec_input_set_prop(struct device *dev, struct input_dev *input_dev, enum input_device_type type, void *data)
{
	if (type == INPUT_DEVICE_TYPE_TOUCH)
		sec_input_set_prop_touch(dev, input_dev, INPUT_PROP_DIRECT, data);
	else if (type == INPUT_DEVICE_TYPE_TOUCHPAD)
		sec_input_set_prop_touch(dev, input_dev, INPUT_PROP_POINTER, data);
	else if (type == INPUT_DEVICE_TYPE_PROXIMITY)
		sec_input_set_prop_proximity(dev, input_dev, data);
	else
		input_err(true, dev, "%s: Don't apply input_device_type! %d\n", __func__, type);
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_set_prop);
#endif

__visible_for_testing void sec_input_set_prop_touch(struct device *dev, struct input_dev *input_dev, u8 propbit, void *data)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	input_dev->phys = input_dev->name;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SW, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(BTN_PALM, input_dev->keybit);
	set_bit(BTN_LARGE_PALM, input_dev->keybit);
	set_bit(KEY_INT_CANCEL, input_dev->keybit);

	set_bit(propbit, input_dev->propbit);
	set_bit(KEY_WAKEUP, input_dev->keybit);
	set_bit(KEY_WATCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(input_dev, SEC_TS_SUPPORT_TOUCH_COUNT, INPUT_MT_POINTER);
	else
		input_mt_init_slots(input_dev, SEC_TS_SUPPORT_TOUCH_COUNT, INPUT_MT_DIRECT);

	input_set_drvdata(input_dev, data);
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL_KUNIT(sec_input_set_prop_touch);
#endif

__visible_for_testing void sec_input_set_prop_proximity(struct device *dev, struct input_dev *input_dev, void *data)
{
	input_dev->phys = input_dev->name;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_SW, input_dev->evbit);

	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_set_abs_params(input_dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	input_set_drvdata(input_dev, data);
}

void sec_input_coord_event_sync_slot(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (pdata->fill_slot)
		efunc.input_sync(pdata->input_dev);
	pdata->fill_slot = false;
}
EXPORT_SYMBOL(sec_input_coord_event_sync_slot);

void sec_input_proximity_report(struct device *dev, int data)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (!pdata->input_dev_proximity)
		return;

	if (pdata->support_lightsensor_detect) {
		if (data == PROX_EVENT_TYPE_LIGHTSENSOR_PRESS || data == PROX_EVENT_TYPE_LIGHTSENSOR_RELEASE) {
			input_report_abs(pdata->input_dev_proximity, ABS_MT_CUSTOM, data);
			efunc.input_sync(pdata->input_dev_proximity);
			input_info(true, dev, "%s: LIGHTSENSOR(%d)\n", __func__, data & 1);
			return;
		}
	}

	if (pdata->support_ear_detect) {
		if (!(pdata->ed_enable || pdata->pocket_mode))
			return;

		input_report_abs(pdata->input_dev_proximity, ABS_MT_CUSTOM, data);
		efunc.input_sync(pdata->input_dev_proximity);
		input_info(true, dev, "%s: PROX(%d)\n", __func__, data);
	}
}
EXPORT_SYMBOL(sec_input_proximity_report);

void sec_input_release_all_finger(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int i;

	if (!pdata->input_dev)
		return;

	if (pdata->touch_count > 0) {
		input_report_key(pdata->input_dev, KEY_INT_CANCEL, 1);
		efunc.input_sync(pdata->input_dev);
		input_report_key(pdata->input_dev, KEY_INT_CANCEL, 0);
		efunc.input_sync(pdata->input_dev);
	}

	if (pdata->input_dev_proximity) {
		input_report_abs(pdata->input_dev_proximity, ABS_MT_CUSTOM, 0xff);
		efunc.input_sync(pdata->input_dev_proximity);
	}

	for (i = 0; i < SEC_TS_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(pdata->input_dev, i);
		input_mt_report_slot_state(pdata->input_dev, MT_TOOL_FINGER, false);

		if (pdata->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS
				|| pdata->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE) {
			sec_input_coord_log(dev, i, SEC_TS_COORDINATE_ACTION_FORCE_RELEASE);
			pdata->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
		}

		pdata->coord[i].mcount = 0;
		pdata->coord[i].palm_count = 0;
		pdata->coord[i].noise_level = 0;
		pdata->coord[i].max_strength = 0;
		pdata->coord[i].hover_id_num = 0;
	}

	input_mt_slot(pdata->input_dev, 0);

	input_report_key(pdata->input_dev, BTN_PALM, false);
	input_report_key(pdata->input_dev, BTN_LARGE_PALM, false);
	input_report_key(pdata->input_dev, BTN_TOUCH, false);
	input_report_key(pdata->input_dev, BTN_TOOL_FINGER, false);
	pdata->palm_flag = 0;
	pdata->touch_count = 0;
	pdata->hw_param.check_multi = 0;

	efunc.input_sync(pdata->input_dev);
}
EXPORT_SYMBOL(sec_input_release_all_finger);

void sec_input_coord_event_fill_slot(struct device *dev, int t_id)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (pdata->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
		if (pdata->prev_coord[t_id].action == SEC_TS_COORDINATE_ACTION_NONE
				|| pdata->prev_coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
			input_err(true, dev,
					"%s: tID %d released without press\n", __func__, t_id);
			return;
		}

		sec_input_coord_report(dev, t_id);
		if ((pdata->touch_count == 0) && !IS_ERR_OR_NULL(&pdata->interrupt_notify_work.work)) {
			if (pdata->interrupt_notify_work.work.func && list_empty(&pdata->interrupt_notify_work.work.entry))
				schedule_work(&pdata->interrupt_notify_work.work);
		}
		sec_input_coord_log(dev, t_id, SEC_TS_COORDINATE_ACTION_RELEASE);

		pdata->coord[t_id].action = SEC_TS_COORDINATE_ACTION_NONE;
		pdata->coord[t_id].mcount = 0;
		pdata->coord[t_id].palm_count = 0;
		pdata->coord[t_id].noise_level = 0;
		pdata->coord[t_id].max_strength = 0;
		pdata->coord[t_id].hover_id_num = 0;
	} else if (pdata->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
		sec_input_coord_report(dev, t_id);
		if ((pdata->touch_count == 1) && !IS_ERR_OR_NULL(&pdata->interrupt_notify_work.work)) {
			if (pdata->interrupt_notify_work.work.func && list_empty(&pdata->interrupt_notify_work.work.entry))
				schedule_work(&pdata->interrupt_notify_work.work);
		}
		sec_input_coord_log(dev, t_id, SEC_TS_COORDINATE_ACTION_PRESS);
	} else if (pdata->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE) {
		if (pdata->prev_coord[t_id].action == SEC_TS_COORDINATE_ACTION_NONE
				|| pdata->prev_coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
			pdata->coord[t_id].action = SEC_TS_COORDINATE_ACTION_PRESS;
			sec_input_coord_report(dev, t_id);
			sec_input_coord_log(dev, t_id, SEC_TS_COORDINATE_ACTION_MOVE);
		} else {
			sec_input_coord_report(dev, t_id);
		}
	} else {
		input_dbg(true, dev,
				"%s: do not support coordinate action(%d)\n",
				__func__, pdata->coord[t_id].action);
	}

	if ((pdata->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS)
			|| (pdata->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
		if (pdata->coord[t_id].ttype != pdata->prev_coord[t_id].ttype) {
			input_info(true, dev, "%s : tID:%d ttype(%x->%x)\n",
					__func__, pdata->coord[t_id].id,
					pdata->prev_coord[t_id].ttype, pdata->coord[t_id].ttype);
		}
	}

	pdata->fill_slot = true;
}
EXPORT_SYMBOL(sec_input_coord_event_fill_slot);

void sec_input_coord_report(struct device *dev, u8 t_id)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int action = pdata->coord[t_id].action;

	if (action == SEC_TS_COORDINATE_ACTION_RELEASE) {
		input_mt_slot(pdata->input_dev, t_id);
		input_mt_report_slot_state(pdata->input_dev, MT_TOOL_FINGER, 0);

		pdata->palm_flag &= ~(1 << t_id);

		if (pdata->touch_count > 0)
			pdata->touch_count--;
		if (pdata->touch_count == 0) {
			input_report_key(pdata->input_dev, BTN_TOUCH, 0);
			input_report_key(pdata->input_dev, BTN_TOOL_FINGER, 0);
			pdata->hw_param.check_multi = 0;
			pdata->print_info_cnt_release = 0;
			pdata->palm_flag = 0;
		}
		if (pdata->blocking_palm)
			input_report_key(pdata->input_dev, BTN_PALM, 0);
		else
			input_report_key(pdata->input_dev, BTN_PALM, pdata->palm_flag);
	} else if (action == SEC_TS_COORDINATE_ACTION_PRESS || action == SEC_TS_COORDINATE_ACTION_MOVE) {
		if (action == SEC_TS_COORDINATE_ACTION_PRESS) {
			pdata->touch_count++;
			pdata->coord[t_id].p_x = pdata->coord[t_id].x;
			pdata->coord[t_id].p_y = pdata->coord[t_id].y;

			pdata->hw_param.all_finger_count++;
			if ((pdata->touch_count > 4) && (pdata->hw_param.check_multi == 0)) {
				pdata->hw_param.check_multi = 1;
				pdata->hw_param.multi_count++;
			}
		} else {
			/* action == SEC_TS_COORDINATE_ACTION_MOVE */
			pdata->coord[t_id].mcount++;
		}

		input_mt_slot(pdata->input_dev, t_id);
		input_mt_report_slot_state(pdata->input_dev, MT_TOOL_FINGER, 1);
		input_report_key(pdata->input_dev, BTN_TOUCH, 1);
		input_report_key(pdata->input_dev, BTN_TOOL_FINGER, 1);
		if (pdata->blocking_palm)
			input_report_key(pdata->input_dev, BTN_PALM, 0);
		else
			input_report_key(pdata->input_dev, BTN_PALM, pdata->palm_flag);

		input_report_abs(pdata->input_dev, ABS_MT_POSITION_X, pdata->coord[t_id].x);
		input_report_abs(pdata->input_dev, ABS_MT_POSITION_Y, pdata->coord[t_id].y);
		input_report_abs(pdata->input_dev, ABS_MT_TOUCH_MAJOR, pdata->coord[t_id].major);
		input_report_abs(pdata->input_dev, ABS_MT_TOUCH_MINOR, pdata->coord[t_id].minor);
	}
}

static void coord_log_press(struct sec_ts_plat_data *pdata, u8 t_id, const char *action);
static void coord_log_release(struct sec_ts_plat_data *pdata, u8 t_id, const char *action);
void sec_input_coord_log(struct device *dev, u8 t_id, int action)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	location_detect(pdata, t_id);

	if (action == SEC_TS_COORDINATE_ACTION_PRESS)
		coord_log_press(pdata, t_id, "P");
	else if (action == SEC_TS_COORDINATE_ACTION_MOVE)
		coord_log_press(pdata, t_id, "M");
	else if (action == SEC_TS_COORDINATE_ACTION_RELEASE)
		coord_log_release(pdata, t_id, "R");
	else if (action == SEC_TS_COORDINATE_ACTION_FORCE_RELEASE)
		coord_log_release(pdata, t_id, "RA");
	else
		input_info(true, dev, "%s: unknown action\n", __func__);
}

static void coord_log_press(struct sec_ts_plat_data *pdata, u8 t_id, const char *action)
{
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, pdata->dev,
			"[%s] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d, fid:%d fod:%x\n",
			action, t_id, input_mt_get_value(&pdata->input_dev->mt->slots[t_id], ABS_MT_TRACKING_ID),
			pdata->coord[t_id].x, pdata->coord[t_id].y, pdata->coord[t_id].z,
			pdata->coord[t_id].major, pdata->coord[t_id].minor,
			pdata->location, pdata->touch_count,
			pdata->coord[t_id].ttype,
			pdata->coord[t_id].noise_status, atomic_read(&pdata->touch_noise_status),
			atomic_read(&pdata->touch_pre_noise_status), pdata->coord[t_id].noise_level,
			pdata->coord[t_id].max_strength, pdata->coord[t_id].hover_id_num,
			pdata->coord[t_id].freq_id, pdata->coord[t_id].fod_debug);
#else
	input_info(true, pdata->dev,
			"[%s] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d, fid:%d fod:%x\n",
			action, t_id, input_mt_get_value(&pdata->input_dev->mt->slots[t_id], ABS_MT_TRACKING_ID),
			pdata->coord[t_id].z, pdata->coord[t_id].major,
			pdata->coord[t_id].minor, pdata->location, pdata->touch_count,
			pdata->coord[t_id].ttype,
			pdata->coord[t_id].noise_status, atomic_read(&pdata->touch_noise_status),
			atomic_read(&pdata->touch_pre_noise_status), pdata->coord[t_id].noise_level,
			pdata->coord[t_id].max_strength, pdata->coord[t_id].hover_id_num,
			pdata->coord[t_id].freq_id, pdata->coord[t_id].fod_debug);
#endif
}

static void coord_log_release(struct sec_ts_plat_data *pdata, u8 t_id, const char *action)
{
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, pdata->dev,
			"[%s] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d, fid:%d fod:%x\n",
			action, t_id, pdata->location,
			pdata->coord[t_id].x - pdata->coord[t_id].p_x,
			pdata->coord[t_id].y - pdata->coord[t_id].p_y,
			pdata->coord[t_id].mcount, pdata->touch_count,
			pdata->coord[t_id].x, pdata->coord[t_id].y,
			pdata->coord[t_id].palm_count,
			pdata->coord[t_id].noise_status, atomic_read(&pdata->touch_noise_status),
			atomic_read(&pdata->touch_pre_noise_status), pdata->coord[t_id].noise_level,
			pdata->coord[t_id].max_strength, pdata->coord[t_id].hover_id_num,
			pdata->coord[t_id].freq_id, pdata->coord[t_id].fod_debug);
#else
	input_info(true, pdata->dev,
			"[%s] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d, fid:%d fod:%x\n",
			action, t_id, pdata->location,
			pdata->coord[t_id].x - pdata->coord[t_id].p_x,
			pdata->coord[t_id].y - pdata->coord[t_id].p_y,
			pdata->coord[t_id].mcount, pdata->touch_count,
			pdata->coord[t_id].palm_count,
			pdata->coord[t_id].noise_status, atomic_read(&pdata->touch_noise_status),
			atomic_read(&pdata->touch_pre_noise_status), pdata->coord[t_id].noise_level,
			pdata->coord[t_id].max_strength, pdata->coord[t_id].hover_id_num,
			pdata->coord[t_id].freq_id, pdata->coord[t_id].fod_debug);
#endif
}
/************************************************************
 *  720  * 1480 : <48 96 60>
 * indicator: 24dp navigator:48dp edge:60px dpi=320
 * 1080  * 2220 :  4096 * 4096 : <133 266 341>  (approximately value)
 ************************************************************/
void location_detect(struct sec_ts_plat_data *pdata, int t_id)
{
	int x = pdata->coord[t_id].x, y = pdata->coord[t_id].y;

	memset(pdata->location, 0x00, SEC_TS_LOCATION_DETECT_SIZE);

	if (x < pdata->area_edge)
		strlcat(pdata->location, "E.", SEC_TS_LOCATION_DETECT_SIZE);
	else if (x < (pdata->max_x - pdata->area_edge))
		strlcat(pdata->location, "C.", SEC_TS_LOCATION_DETECT_SIZE);
	else
		strlcat(pdata->location, "e.", SEC_TS_LOCATION_DETECT_SIZE);

	if (y < pdata->area_indicator)
		strlcat(pdata->location, "S", SEC_TS_LOCATION_DETECT_SIZE);
	else if (y < (pdata->max_y - pdata->area_navigation))
		strlcat(pdata->location, "C", SEC_TS_LOCATION_DETECT_SIZE);
	else
		strlcat(pdata->location, "N", SEC_TS_LOCATION_DETECT_SIZE);
}

MODULE_DESCRIPTION("Samsung input common reportdata");
MODULE_LICENSE("GPL");