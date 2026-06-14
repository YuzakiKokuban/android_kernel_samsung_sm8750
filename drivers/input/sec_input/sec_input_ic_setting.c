#include "sec_input.h"

#if IS_ENABLED(CONFIG_UML)
__visible_for_testing enum sec_ts_error sec_input_set_temperature_data(struct device *dev, int force);
__visible_for_testing DECLARE_REDIRECT_MOCKABLE(sec_input_temperature_state_check,
			RETURNS(enum sec_ts_error),
			PARAMS(struct device *, enum set_temperature_state));
__visible_for_testing DECLARE_REDIRECT_MOCKABLE(sec_input_get_power_supply, RETURNS(enum sec_ts_error), PARAMS(struct device *));
#else
__visible_for_testing enum sec_ts_error sec_input_set_temperature_data(struct device *dev, int force);
__visible_for_testing enum sec_ts_error sec_input_temperature_state_check(struct device *dev,
						enum set_temperature_state state);
__visible_for_testing enum sec_ts_error sec_input_get_power_supply(struct device *dev);
#endif

enum sec_ts_error sec_input_set_temperature(struct device *dev, int state)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int ret = 0;
	enum sec_ts_error state_result = SEC_ERROR;

	if (pdata->set_temperature == NULL) {
		input_dbg(false, dev, "%s: vendor function is not allocated\n", __func__);
		return SEC_ERROR;
	}

	state_result = sec_input_temperature_state_check(dev, state);
	if (state_result == SEC_SKIP || state_result == SEC_ERROR) {
		input_dbg(false, dev, "%s: state check %s t_cnt:%d\n", __func__,
		state_result == SEC_SKIP ? "skip" : "failed", pdata->touch_count);
		return state_result;
	}

	pdata->tsp_temperature_data_skip = false;
	ret = sec_input_get_power_supply(dev);
	if (ret == SEC_ERROR) {
		input_err(true, dev, "%s: couldn't get temperature value, ret:%d\n", __func__, ret);
		return ret;
	}

	return sec_input_set_temperature_data(dev, state_result);
}
EXPORT_SYMBOL(sec_input_set_temperature);

#if IS_ENABLED(CONFIG_UML)
__visible_for_testing DEFINE_REDIRECT_MOCKABLE(sec_input_temperature_state_check, RETURNS(enum sec_ts_error), PARAMS(struct device *, enum set_temperature_state));
__visible_for_testing enum sec_ts_error REAL_ID(sec_input_temperature_state_check)(struct device *dev,
						enum set_temperature_state state)
#else
__visible_for_testing enum sec_ts_error sec_input_temperature_state_check(struct device *dev,
						enum set_temperature_state state)
#endif
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	switch (state) {
	case SEC_INPUT_SET_TEMPERATURE_FORCE:
		return SEC_FORCE;
	case SEC_INPUT_SET_TEMPERATURE_NORMAL:
		if (pdata->touch_count) {
			pdata->tsp_temperature_data_skip = true;
			input_err(true, dev, "%s: skip, t_cnt(%d)\n",
					__func__, pdata->touch_count);
			return SEC_SKIP;
		}
		break;
	case SEC_INPUT_SET_TEMPERATURE_IN_IRQ:
		if (pdata->touch_count != 0 || pdata->tsp_temperature_data_skip == false)
			return SEC_SKIP;
		break;
	default:
		input_err(true, dev, "%s: invalid param %d\n", __func__, state);
		return SEC_ERROR;
	}

	return SEC_SUCCESS;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_temperature_state_check);
#endif

#if IS_ENABLED(CONFIG_UML)
__visible_for_testing DEFINE_REDIRECT_MOCKABLE(sec_input_get_power_supply, RETURNS(enum sec_ts_error), PARAMS(struct device *));
__visible_for_testing enum sec_ts_error REAL_ID(sec_input_get_power_supply)(struct device *dev)
#else
__visible_for_testing enum sec_ts_error sec_input_get_power_supply(struct device *dev)
#endif
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	enum sec_ts_error ret;

	if (!pdata->psy)
		pdata->psy = efunc.power_supply_get_by_name("battery");

	if (!pdata->psy) {
		input_err(true, pdata->dev, "%s: cannot find power supply\n", __func__);
		return SEC_ERROR;
	}

	ret = efunc.power_supply_get_property(pdata->psy, POWER_SUPPLY_PROP_TEMP, &pdata->psy_value);
	if (ret < 0) {
		input_err(true, dev, "%s: couldn't get temperature value, ret:%d\n", __func__, ret);
		return SEC_ERROR;
	}

	return SEC_SUCCESS;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_get_power_supply);
#endif

__visible_for_testing enum sec_ts_error sec_input_set_temperature_data(struct device *dev, int force)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	enum sec_ts_error ret;
	u8 temperature_data;

	temperature_data = (u8)(pdata->psy_value.intval / 10);

	if (force || pdata->tsp_temperature_data != temperature_data) {
		ret = pdata->set_temperature(dev, temperature_data);
		if (ret < 0) {
			input_err(true, dev, "%s: failed to write temperature %u, ret=%d\n",
				__func__, temperature_data, ret);
			return SEC_ERROR;
		}

		pdata->tsp_temperature_data = temperature_data;
		input_info(true, dev, "%s set temperature:%u\n", __func__, temperature_data);
	} else {
		input_dbg(true, dev, "%s skip temperature:%u\n", __func__, temperature_data);
	}

	return SEC_SUCCESS;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_set_temperature_data);
#endif

void sec_input_set_grip_type(struct device *dev, u8 set_type)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	u8 mode = G_NONE;

	if (pdata->set_grip_data == NULL) {
		input_dbg(true, dev, "%s: vendor function is not allocated\n", __func__);
		return;
	}

	input_info(true, dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, pdata->grip_data.edgehandler_direction,
			pdata->grip_data.edge_range, pdata->grip_data.landscape_mode);

	if (pdata->grip_data.edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (pdata->grip_data.edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone default 0 mode, 32 */
		if (pdata->grip_data.landscape_mode == 1)
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		pdata->set_grip_data(dev, mode);
}
EXPORT_SYMBOL(sec_input_set_grip_type);

__visible_for_testing int sec_input_store_grip_data_edge(struct sec_ts_plat_data *pdata, int *cmd_param);
__visible_for_testing int sec_input_store_grip_data_portrait(struct sec_ts_plat_data *pdata, int *cmd_param);
__visible_for_testing int sec_input_store_grip_data_landscape(struct sec_ts_plat_data *pdata, int *cmd_param);
static bool store_grip_data_cmd_verify(struct sec_ts_plat_data *pdata, int *cmd_param);
int sec_input_store_grip_data(struct device *dev, int *cmd_param)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int mode = G_NONE;

	if (!pdata)
		return SEC_ERROR;

	if (cmd_param[0] == 0) {
		mode = sec_input_store_grip_data_edge(pdata, &cmd_param[1]);
	} else if (cmd_param[0] == 1) {
		mode = sec_input_store_grip_data_portrait(pdata, &cmd_param[1]);
	} else if (cmd_param[0] == 2) {
		mode = sec_input_store_grip_data_landscape(pdata, &cmd_param[1]);
	} else {
		input_err(true, dev, "%s: cmd0 is abnormal, %d", __func__, cmd_param[0]);
		return SEC_ERROR;
	}

	return mode;
}
EXPORT_SYMBOL(sec_input_store_grip_data);

__visible_for_testing int sec_input_store_grip_data_edge(struct sec_ts_plat_data *pdata, int *cmd_param)
{
	if (!store_grip_data_cmd_verify(pdata, cmd_param))
		return SEC_ERROR;

	if (cmd_param[0] == 0) {
		pdata->grip_data.edgehandler_direction = 0;
		input_info(true, pdata->dev, "%s: [edge handler] clear\n", __func__);
	} else if (cmd_param[0] < 5) {
		pdata->grip_data.edgehandler_direction = cmd_param[0];
		pdata->grip_data.edgehandler_start_y = cmd_param[1];
		pdata->grip_data.edgehandler_end_y = cmd_param[2];
		input_info(true, pdata->dev, "%s: [edge handler] dir:%d, range:%d,%d\n", __func__,
				pdata->grip_data.edgehandler_direction,
				pdata->grip_data.edgehandler_start_y,
				pdata->grip_data.edgehandler_end_y);
	} else {
		input_err(true, pdata->dev, "%s: [edge handler] cmd1 is abnormal, %d\n", __func__, cmd_param[0]);
		return SEC_ERROR;
	}

	return G_NONE | G_SET_EDGE_HANDLER;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_store_grip_data_edge);
#endif

__visible_for_testing int sec_input_store_grip_data_portrait(struct sec_ts_plat_data *pdata, int *cmd_param)
{
	int mode = G_NONE;

	if (!store_grip_data_cmd_verify(pdata, cmd_param))
		return SEC_ERROR;

	if (pdata->grip_data.edge_range != cmd_param[0])
		mode = mode | G_SET_EDGE_ZONE;
	pdata->grip_data.edge_range = cmd_param[0];
	pdata->grip_data.deadzone_up_x = cmd_param[1];
	pdata->grip_data.deadzone_dn_x = cmd_param[2];
	pdata->grip_data.deadzone_y = cmd_param[3];
	/* 3rd reject zone */
	pdata->grip_data.deadzone_dn2_x = cmd_param[4];
	pdata->grip_data.deadzone_dn_y = cmd_param[5];
	mode |= G_SET_NORMAL_MODE;

	if (pdata->grip_data.landscape_mode == 1) {
		pdata->grip_data.landscape_mode = 0;
		mode |= G_CLR_LANDSCAPE_MODE;
	}

	/*
	* w means width of divided by 3 zone (X1, X2, X3)
	* h means height of divided by 3 zone (Y1, Y2) - y coordinate which is the point of divide line
	*/
	input_info(true, pdata->dev, "%s: [%sportrait] grip:%d | reject w:%d/%d/%d, h:%d/%d\n",
		__func__, (mode & G_CLR_LANDSCAPE_MODE) ? "landscape->" : "",
		pdata->grip_data.edge_range, pdata->grip_data.deadzone_up_x,
		pdata->grip_data.deadzone_dn_x, pdata->grip_data.deadzone_dn2_x,
		pdata->grip_data.deadzone_y, pdata->grip_data.deadzone_dn_y);

	return mode;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_store_grip_data_portrait);
#endif

__visible_for_testing int sec_input_store_grip_data_landscape(struct sec_ts_plat_data *pdata, int *cmd_param)
{
	if (!store_grip_data_cmd_verify(pdata, cmd_param))
		return SEC_ERROR;


	if (cmd_param[0] == 0) {
		pdata->grip_data.landscape_mode = 0;
		input_info(true, pdata->dev, "%s: [landscape] clear\n", __func__);
		return G_NONE | G_CLR_LANDSCAPE_MODE;
	} else if (cmd_param[0] == 1) {
		pdata->grip_data.landscape_mode = 1;
		pdata->grip_data.landscape_edge = cmd_param[1];
		pdata->grip_data.landscape_deadzone = cmd_param[2];
		pdata->grip_data.landscape_top_deadzone = cmd_param[3];
		pdata->grip_data.landscape_bottom_deadzone = cmd_param[4];
		pdata->grip_data.landscape_top_gripzone = cmd_param[5];
		pdata->grip_data.landscape_bottom_gripzone = cmd_param[6];
		/*
		* v means width of grip/reject zone of vertical both edge side
		* h means height of grip/reject zone of horizontal top/bottom side (top/bottom)
		*/
		input_info(true, pdata->dev, "%s: [landscape] grip v:%d, h:%d/%d | reject v:%d, h:%d/%d\n",
				__func__, pdata->grip_data.landscape_edge,
				pdata->grip_data.landscape_top_gripzone, pdata->grip_data.landscape_bottom_gripzone,
				pdata->grip_data.landscape_deadzone, pdata->grip_data.landscape_top_deadzone,
				pdata->grip_data.landscape_bottom_deadzone);
		return G_NONE | G_SET_LANDSCAPE_MODE;
	}

	input_err(true, pdata->dev, "%s: [landscape] cmd1 is abnormal, %d\n", __func__, cmd_param[0]);
	return SEC_ERROR;
}
#if IS_ENABLED(CONFIG_UML)
EXPORT_SYMBOL(sec_input_store_grip_data_landscape);
#endif

static bool store_grip_data_cmd_verify(struct sec_ts_plat_data *pdata, int *cmd_param)
{
	if (cmd_param[0] < 0
		|| cmd_param[1] < 0
		|| cmd_param[2] < 0
		|| cmd_param[3] < 0
		|| cmd_param[4] < 0
		|| cmd_param[5] < 0
		|| cmd_param[6] < 0) {
		input_err(true, pdata->dev, "%s: cmd1 is abnormal data:%d %d %d %d %d %d %d\n",
				__func__, cmd_param[0], cmd_param[1], cmd_param[2], cmd_param[3],
				cmd_param[4], cmd_param[5], cmd_param[6]);
		return false;
	}
	return true;
}

void sec_input_set_fod_info(struct device *dev, int vi_x, int vi_y, int vi_size, int vi_event)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int byte_size = vi_x * vi_y / 8;

	if (vi_x * vi_y % 8)
		byte_size++;

	pdata->fod_data.vi_x = vi_x;
	pdata->fod_data.vi_y = vi_y;
	pdata->fod_data.vi_size = vi_size;
	pdata->fod_data.vi_event = vi_event;

	if (byte_size != vi_size)
		input_err(true, dev, "%s: NEED TO CHECK! vi size %d maybe wrong (byte size should be %d)\n",
				__func__, vi_size, byte_size);

	input_info(true, dev, "%s: vi_event:%d, x:%d, y:%d, size:%d\n",
			__func__, pdata->fod_data.vi_event, pdata->fod_data.vi_x, pdata->fod_data.vi_y,
			pdata->fod_data.vi_size);
}
EXPORT_SYMBOL(sec_input_set_fod_info);

ssize_t sec_input_get_fod_info(struct device *dev, char *buf)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (!pdata->support_fod) {
		input_err(true, dev, "%s: fod is not supported\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NA");
	}

	if (pdata->x_node_num <= 0 || pdata->y_node_num <= 0) {
		input_err(true, dev, "%s: x/y node num value is wrong\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	input_info(true, dev, "%s: x:%d/%d, y:%d/%d, size:%d\n",
			__func__, pdata->fod_data.vi_x, pdata->x_node_num,
			pdata->fod_data.vi_y, pdata->y_node_num, pdata->fod_data.vi_size);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d",
			pdata->fod_data.vi_x, pdata->fod_data.vi_y,
			pdata->fod_data.vi_size, pdata->x_node_num, pdata->y_node_num);
}
EXPORT_SYMBOL(sec_input_get_fod_info);

bool sec_input_set_fod_rect(struct device *dev, int *rect_data)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int i;

	pdata->fod_data.set_val = 1;

	if (rect_data[0] <= 0 || rect_data[1] <= 0 || rect_data[2] <= 0 || rect_data[3] <= 0)
		pdata->fod_data.set_val = 0;

	if (pdata->fod_data.set_val)
		for (i = 0; i < 4; i++)
			pdata->fod_data.rect_data[i] = rect_data[i];

	return pdata->fod_data.set_val;
}
EXPORT_SYMBOL(sec_input_set_fod_rect);

static bool set_wirelesscharger_mode_verify(enum wireless_charger_param mode);
static enum sec_ts_error set_wirelesscharger_mode_force(struct device *dev, enum wireless_charger_param mode);
enum sec_ts_error sec_input_check_wirelesscharger_mode(struct device *dev, enum wireless_charger_param mode, int force)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (!set_wirelesscharger_mode_verify(mode)) {
		input_err(true, dev,
				"%s: invalid param %d\n", __func__, mode);
		return SEC_ERROR;
	}

	if (force)
		return set_wirelesscharger_mode_force(dev, mode);

	if (pdata->force_wirelesscharger_mode == true) {
		input_err(true, dev,
				"%s: [force enable] skip %d\n", __func__, mode);
		return SEC_SKIP;
	}

	pdata->wirelesscharger_mode = mode & 0xFF;

	return SEC_SUCCESS;
}
EXPORT_SYMBOL(sec_input_check_wirelesscharger_mode);

static bool set_wirelesscharger_mode_verify(enum wireless_charger_param mode)
{
	switch (mode) {
	case TYPE_WIRELESS_CHARGER_NONE:
	case TYPE_WIRELESS_CHARGER:
	case TYPE_WIRELESS_BATTERY_PACK:
		return true;
	default:
		return false;
	}
}

static enum sec_ts_error set_wirelesscharger_mode_force(struct device *dev, enum wireless_charger_param mode)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;

	if (mode == TYPE_WIRELESS_CHARGER_NONE) {
		pdata->force_wirelesscharger_mode = false;
		input_err(true, dev,
				"%s: force enable off\n", __func__);
		return SEC_SKIP;
	}

	pdata->force_wirelesscharger_mode = true;
	pdata->wirelesscharger_mode = mode & 0xFF;

	return SEC_SUCCESS;
}
MODULE_DESCRIPTION("Samsung input ic setting");
MODULE_LICENSE("GPL");
