/* SPDX-License-Identifier: GPL-2.0-only */
/* drivers/input/sec_input/sec_input_multi_dev.c
 *
 * Core file for Samsung input device driver for multi device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_input_multi_dev.h"

bool sec_input_need_fold_off(struct sec_input_multi_device *mdev)
{
	struct sec_ts_plat_data *plat_data;

	plat_data = mdev->dev->platform_data;
	if (!plat_data) {
		input_err(true, mdev->dev, "%s: no platform data\n", __func__);
		return false;
	}

	if (IS_NOT_FOLD_DEV(mdev))
		return false;

	if ((mdev->device_count == MULTI_DEV_MAIN) && (mdev->flip_status_current == FOLD_STATUS_FOLDING))
		return true;

	if ((mdev->device_count == MULTI_DEV_SUB) &&
		(mdev->flip_status_current == FOLD_STATUS_UNFOLDING) && !plat_data->always_lpm)
		return true;

	return false;
}
EXPORT_SYMBOL(sec_input_need_fold_off);

void sec_input_set_fold_state(struct sec_input_multi_device *mdev, int state)
{
	if (IS_NOT_FOLD_DEV(mdev))
		return;

	cancel_delayed_work(&mdev->switching_work);
	mdev->flip_status_current = state;
	schedule_work(&mdev->switching_work.work);
	input_info(true, mdev->dev, "%s: [%s] state:%d %sfolding\n",
					__func__, mdev->name,
					state, mdev->flip_status_current ? "" : "un");
}
EXPORT_SYMBOL(sec_input_set_fold_state);

void sec_input_multi_device_ready(struct sec_input_multi_device *mdev)
{
	if (IS_NOT_FOLD_DEV(mdev))
		return;

	mdev->device_ready = true;

	if (mdev->change_flip_status) {
		input_info(true, mdev->dev, "%s: re-try switching\n", __func__);
		schedule_work(&mdev->switching_work.work);
	}
}
EXPORT_SYMBOL(sec_input_multi_device_ready);

void sec_input_get_multi_device_debug_info(struct sec_input_multi_device *mdev, char *buf, ssize_t size)
{
	char mbuff[MULTI_DEV_DEBUG_INFO_SIZE] = { 0 };

	if (IS_NOT_FOLD_DEV(mdev))
		return;

	snprintf(mbuff, sizeof(mbuff), "Multi Device Info(%s)\n", mdev->name);
	strlcat(buf, mbuff, size);
	snprintf(mbuff, sizeof(mbuff), "- status:%d / current:%d\n", mdev->flip_status, mdev->flip_status_current);
	strlcat(buf, mbuff, size);
	snprintf(mbuff, sizeof(mbuff), "- flip_mismatch_count:%d\n", mdev->flip_mismatch_count);
	strlcat(buf, mbuff, size);
}
EXPORT_SYMBOL(sec_input_get_multi_device_debug_info);

void sec_input_check_fold_status(struct sec_input_multi_device *mdev, bool flip_changed)
{
	struct sec_ts_plat_data *plat_data;
	void *data;

	if (IS_NOT_FOLD_DEV(mdev) || !mdev->dev)
		return;

	data = dev_get_drvdata(mdev->dev);
	if (!data) {
		input_err(true, mdev->dev, "%s: no drvdata\n", __func__);
		return;
	}

	plat_data = mdev->dev->platform_data;
	if (!plat_data) {
		input_err(true, mdev->dev, "%s: no platform data\n", __func__);
		return;
	}

	if (!plat_data->stop_device || !plat_data->start_device || !plat_data->lpmode) {
		input_err(true, mdev->dev, "%s: func pointer is not connected for platform data\n", __func__);
		return;
	}

	input_info(true, mdev->dev, "%s: [%s] %s%sfolding\n", __func__, mdev->name,
			flip_changed ? "changed to " : "",
			(mdev->flip_status_current == FOLD_STATUS_FOLDING) ? "" : "un");

	if (plat_data->prox_power_off) {
		plat_data->prox_power_off = 0;
		input_info(true, mdev->dev, "%s: clear prox_power_off\n", __func__);
	}

	switch (mdev->device_count) {
	case MULTI_DEV_MAIN:
		if (flip_changed) {
			if (mdev->flip_status_current == FOLD_STATUS_FOLDING) {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)) {
					input_info(true, mdev->dev, "%s: [%s] TSP IC LP => IC OFF\n",
							__func__, mdev->name);
					plat_data->stop_device(data);
				}
			}
		} else {
			mdev->flip_mismatch_count++;
			if (mdev->flip_status_current == FOLD_STATUS_UNFOLDING) {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_POWEROFF) && plat_data->lowpower_mode) {
					input_info(true, mdev->dev, "%s: [%s] TSP IC OFF => LP mode[0x%X]\n",
							__func__, mdev->name, plat_data->lowpower_mode);
					plat_data->start_device(data);
					plat_data->lpmode(data, TO_LOWPOWER_MODE);
				} else if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)
						&& (plat_data->lowpower_mode == 0)) {
					input_info(true, mdev->dev, "%s: [%s] LP mode [0x0] => TSP IC OFF\n",
							__func__, mdev->name);
					plat_data->stop_device(data);
				} else {
					input_info(true, mdev->dev, "%s: [%s] reinit, LP[0x%X]\n",
							__func__, mdev->name, plat_data->lowpower_mode);
					if (plat_data->init)
						plat_data->init(data);
					else
						input_err(true, mdev->dev, "%s: [%s] no init function. do nothing\n",
								__func__, mdev->name);
				}
			}
		}
		break;
	case MULTI_DEV_SUB:
		if (flip_changed) {
			if (mdev->flip_status_current == FOLD_STATUS_FOLDING) {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_POWEROFF) && plat_data->lowpower_mode) {
					input_info(true, mdev->dev, "%s: [%s] TSP IC OFF => LP[0x%X]\n",
							__func__, mdev->name, plat_data->lowpower_mode);
					plat_data->start_device(data);
					plat_data->lpmode(data, TO_LOWPOWER_MODE);
				}
			} else {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)) {
					if (sec_input_need_fold_off(mdev)) {
						input_info(true, mdev->dev, "%s: [%s] TSP IC LP => IC OFF\n",
								__func__, mdev->name);
						plat_data->stop_device(data);
					} else {
						input_info(true, mdev->dev, "%s: [%s] TSP IC LP => on going\n",
								__func__, mdev->name);
					}
				} else if (sec_input_cmp_ic_status(mdev->dev, CHECK_POWEROFF)) {
					if (!sec_input_need_fold_off(mdev)) {
						input_info(true, mdev->dev, "%s: [%s] TSP IC OFF => LP[0x%X]\n",
								__func__, mdev->name, plat_data->lowpower_mode);
						plat_data->start_device(data);
						plat_data->lpmode(data, TO_LOWPOWER_MODE);
					}
				}
			}
		} else {
			mdev->flip_mismatch_count++;
			if (mdev->flip_status_current == FOLD_STATUS_FOLDING) {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_POWEROFF) && plat_data->lowpower_mode) {
					input_info(true, mdev->dev, "%s: [%s] TSP IC OFF => LP[0x%X]\n",
							__func__, mdev->name, plat_data->lowpower_mode);
					plat_data->start_device(data);
					plat_data->lpmode(data, TO_LOWPOWER_MODE);
				} else if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)
						&& plat_data->lowpower_mode == 0) {
					input_info(true, mdev->dev, "%s: [%s] LP mode [0x0] => IC OFF\n",
							__func__, mdev->name);
					plat_data->stop_device(data);
				} else if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)
						&& plat_data->lowpower_mode) {
					/*
					 * CAUTION!
					 * need to check there is no problem with duplicated call of lpmode function
					 */
					input_info(true, mdev->dev, "%s: [%s] call LP mode again\n",
							__func__, mdev->name);
					plat_data->lpmode(data, TO_LOWPOWER_MODE);
				}
			} else {
				if (sec_input_cmp_ic_status(mdev->dev, CHECK_LPMODE)) {
					if (sec_input_need_fold_off(mdev)) {
						input_info(true, mdev->dev, "%s: [%s] TSP IC LP => IC OFF\n",
								__func__, mdev->name);
						plat_data->stop_device(data);
					} else {
						input_info(true, mdev->dev, "%s: [%s] TSP IC LP => on going\n",
								__func__, mdev->name);
					}
				} else if (sec_input_cmp_ic_status(mdev->dev, CHECK_POWEROFF)) {
					if (!sec_input_need_fold_off(mdev)) {
						input_info(true, mdev->dev, "%s: [%s] TSP IC OFF => LP[0x%X]\n",
								__func__, mdev->name, plat_data->lowpower_mode);
						plat_data->start_device(data);
						plat_data->lpmode(data, TO_LOWPOWER_MODE);
					}
				}
			}

		}
		break;
	default:
		break;
	}
}

void sec_input_switching_work(struct work_struct *work)
{
	struct sec_input_multi_device *mdev = container_of(work, struct sec_input_multi_device,
				switching_work.work);
	struct sec_ts_plat_data *plat_data;
	bool flip_changed = false;

	if (mdev == NULL) {
		input_err(true, NULL, "%s: multi device is null\n", __func__);
		return;
	}

	if (IS_NOT_FOLD_DEV(mdev) || !mdev->dev)
		return;

	plat_data = mdev->dev->platform_data;
	if (!plat_data || !plat_data->input_dev) {
		input_err(true, mdev->dev, "%s: no platform data\n", __func__);
		return;
	}

	if (mutex_lock_interruptible(&plat_data->enable_mutex)) {
		input_err(true, mdev->dev, "%s: failed to lock enable_mutex\n", __func__);
		return;
	}

	if (mdev->flip_status != mdev->flip_status_current)
		flip_changed = true;

	if (flip_changed && !mdev->device_ready) {
		input_err(true, mdev->dev, "%s: device is not ready yet\n", __func__);
		mdev->change_flip_status = 1;
		goto out;
	}

	mdev->change_flip_status = 0;
	if (plat_data->sec_ws)
		__pm_stay_awake(plat_data->sec_ws);
	sec_input_check_fold_status(mdev, flip_changed);
	if (plat_data->sec_ws)
		__pm_relax(plat_data->sec_ws);
	mdev->flip_status = mdev->flip_status_current;
	input_info(true, mdev->dev, "%s: [%s] done\n", __func__, mdev->name);
out:
	mutex_unlock(&plat_data->enable_mutex);
}

/*
 * sec_input_multi_device_create
 * struct device *dev :
 *			it should be a (i2c/spi)client->dev.
 *			driver data should be set as drvdata.
 *			sec_ts_plat_data should be connected as dev->platform_data.
 *			start_device, stop_device, lpmode function pointer should be set in platform_data.
 */
void sec_input_multi_device_create(struct device *dev)
{
	struct sec_ts_plat_data *pdata;
	struct sec_input_multi_device *multi_dev;
	int device_count;

	device_count = sec_input_multi_device_parse_dt(dev);
	if (IS_NOT_FOLD(device_count))
		return;

	pdata = dev->platform_data;
	if (!pdata) {
		input_err(true, dev, "%s: no platform data\n", __func__);
		return;
	}

	input_info(true, dev, "%s for %s\n", __func__, GET_FOLD_STR(device_count));
	multi_dev = devm_kzalloc(dev, sizeof(struct sec_input_multi_device), GFP_KERNEL);
	if (!multi_dev)
		return;

	multi_dev->dev = dev;
	multi_dev->device_count = device_count;
	multi_dev->name = GET_FOLD_STR(device_count);
	multi_dev->flip_status = -1;
	if (device_count == MULTI_DEV_SUB)
		multi_dev->flip_status_current = FOLD_STATUS_FOLDING;
	else
		multi_dev->flip_status_current = FOLD_STATUS_UNFOLDING;

	INIT_DELAYED_WORK(&multi_dev->switching_work, sec_input_switching_work);

	pdata->multi_dev = multi_dev;
}
EXPORT_SYMBOL(sec_input_multi_device_create);

void sec_input_multi_device_remove(struct sec_input_multi_device *mdev)
{
	if (!mdev || !mdev->dev)
		return;

	input_info(true, mdev->dev, "%s\n", __func__);
	cancel_delayed_work_sync(&mdev->switching_work);
}
EXPORT_SYMBOL(sec_input_multi_device_remove);

MODULE_DESCRIPTION("Samsung input multi device");
MODULE_LICENSE("GPL");
