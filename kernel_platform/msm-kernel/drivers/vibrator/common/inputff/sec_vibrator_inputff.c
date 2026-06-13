// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* sec vibrator inputff */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/vibrator/sec_vibrator_inputff.h>

#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif

static struct sec_vib_inputff_pdata  sec_vib_inputff_pdata;

static struct blocking_notifier_head sec_vib_inputff_nb_head =
	BLOCKING_NOTIFIER_INIT(sec_vib_inputff_nb_head);

int sec_vib_inputff_notifier_register(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = blocking_notifier_chain_register(&sec_vib_inputff_nb_head, nb);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_notifier_register);

int sec_vib_inputff_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = blocking_notifier_chain_unregister(&sec_vib_inputff_nb_head, nb);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_notifier_unregister);

int sec_vib_inputff_vib_notifier_notify(void)
{
	int ret = 0;

	ret = blocking_notifier_call_chain(&sec_vib_inputff_nb_head,
			VIB_NOTIFIER_ON, NULL);

	switch (ret) {
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s failed(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_vib_notifier_notify);

__visible_for_testing bool sec_vib_inputff_getbit(struct sec_vib_inputff_drvdata *ddata, int val)
{
	return (ddata->ff_val >> (val - FF_EFFECT_MIN) & 1);
}

int sec_vib_inputff_setbit(struct sec_vib_inputff_drvdata *ddata, int val)
{
	return ddata->ff_val |= (1 << (val - FF_EFFECT_MIN));
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_setbit);

static void configure_constant_effect(struct sec_vib_inputff_drvdata *ddata,
		struct common_inputff_effect *inputff_effect, int index)
{
	if (!ddata || !inputff_effect)
		return;

	ddata->effects[index].gain = (ddata->effect_gain * inputff_effect->scale) / 100;
	ddata->effects[index].compose_effects.type = inputff_effect->type;
	ddata->effects[index].compose_effects.id = ddata->compose.compose_effect_id;
	ddata->effects[index].compose_effects.replay.length = inputff_effect->duration;
	ddata->effects[index].compose_effects.u.constant.level = inputff_effect->frequency;
	pr_info("%s <%d> gain:%d type:%d duration:%d\n", __func__, index,
		ddata->effects[index].gain, ddata->effects[index].compose_effects.type,
		ddata->effects[index].compose_effects.replay.length);
}

static void configure_periodic_effect(struct sec_vib_inputff_drvdata *ddata,
		struct common_inputff_effect *inputff_effect, int index)
{
	if (!ddata || !inputff_effect)
		return;

	ddata->effects[index].gain = (ddata->effect_gain * inputff_effect->scale) / 100;
	ddata->effects[index].compose_effects.type = inputff_effect->type;
	ddata->effects[index].compose_effects.id = ddata->compose.compose_effect_id;
	ddata->effects[index].compose_effects.replay.length = inputff_effect->duration;
	ddata->effects[index].compose_effects.u.periodic.offset = inputff_effect->effect_id;
	ddata->effects[index].compose_effects.u.periodic.waveform = 0;
	pr_info("%s <%d> gain:%d type:%d sep index:%d duration:%d\n", __func__, index,
		ddata->effects[index].gain, ddata->effects[index].compose_effects.type,
		ddata->effects[index].compose_effects.u.periodic.offset,
		ddata->effects[index].compose_effects.replay.length);
}

static void configure_vib_free_duration(struct sec_vib_inputff_drvdata *ddata,
		struct common_inputff_effect *inputff_effect, int index)
{
	if (!ddata || !inputff_effect)
		return;

	ddata->effects[index].compose_effects.type = 0;
	ddata->effects[index].compose_effects.replay.length = inputff_effect->duration;
	pr_info("%s <%d> type:%d duration:%d\n", __func__, index,
		ddata->effects[index].compose_effects.type,
		ddata->effects[index].compose_effects.replay.length);
}

__visible_for_testing int parsing_compose_effects(struct sec_vib_inputff_drvdata *ddata,
		struct common_inputff_effects *input_effects)
{
	struct common_inputff_effect *inputff_effect;
	int i = 0, ret = 0;

	if (!ddata) {
		ret = -ENOENT;
		pr_err("%s ddata null\n", __func__);
		goto err;
	}

	if (!input_effects) {
		ret = -ENOENT;
		pr_err("%s input_effects null\n", __func__);
		goto err;
	}

	ddata->compose.num_of_compose_effects = input_effects->num_of_effects;
	ddata->compose.compose_repeat = input_effects->repeat;
	ddata->compose.pattern_idx = 0;

	pr_info("%s num_of_effects:%d repeat:%d\n", __func__,
		ddata->compose.num_of_compose_effects, ddata->compose.compose_repeat);

	if (ddata->compose.num_of_compose_effects > MAX_COMPOSE_EFFECT) {
		pr_err("%s error. num_of_compose_effects=%d\n", __func__,
			ddata->compose.num_of_compose_effects);
		ret = -EFBIG;
		goto err;
	}

	for (i = 0; i < ddata->compose.num_of_compose_effects; i++) {
		inputff_effect = &input_effects->effects[i];

		switch (inputff_effect->type) {
		case FF_CONSTANT:
			configure_constant_effect(ddata, inputff_effect, i);
			break;
		case FF_PERIODIC:
			configure_periodic_effect(ddata, inputff_effect, i);
			break;
		case VIB_FREE_DURATION:
			configure_vib_free_duration(ddata, inputff_effect, i);
			break;
		default:
			pr_err("%s invalid effect type=%d\n", __func__, inputff_effect->type);
			ret = -EINVAL;
			goto err;
		}
	}
err:
	return ret;
}

__visible_for_testing void compose_effect_set_gain(struct input_dev *dev, u16 gain)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);

	pr_info("%s\n", __func__);

	if (!ddata)
		return;

	if (ddata && ddata->vib_ops->set_gain)
		ddata->vib_ops->set_gain(dev, gain);
}

__visible_for_testing int compose_effect_upload(struct input_dev *dev, struct ff_effect *effect)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!ddata || !effect) {
		ret = -ENOENT;
		goto err;
	}

	if (ddata && ddata->vib_ops->upload) {
		ret = ddata->vib_ops->upload(dev, effect, NULL);
		if (ret) {
			pr_err("%s error. upload ret=%d\n", __func__, ret);
			goto err;
		}
	}
	ddata->compose.upload_partial_effect = 1;
err:
	return ret;
}

__visible_for_testing int compose_effect_playback_on(struct input_dev *dev,
		struct ff_effect *effect)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!ddata || !effect) {
		ret = -ENOENT;
		goto err;
	}

	if (ddata && ddata->vib_ops->playback) {
		ret = ddata->vib_ops->playback(dev, effect->id, 1);
		if (ret)
			pr_err("%s error. ret=%d\n", __func__, ret);
	}
err:
	return ret;
}

__visible_for_testing int compose_effect_playback_off(struct input_dev *dev,
		struct ff_effect *effect)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!ddata || !effect) {
		ret = -ENOENT;
		goto err;
	}

	if (ddata && ddata->vib_ops->playback) {
		ret = ddata->vib_ops->playback(dev, effect->id, 0);
		if (ret)
			pr_err("%s error. ret=%d\n", __func__, ret);
	}
err:
	return ret;
}

__visible_for_testing int compose_effect_erase(struct input_dev *dev, int effect_id)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!ddata) {
		ret = -ENOENT;
		goto err;
	}

	if (ddata->vib_ops->erase) {
		ret = ddata->vib_ops->erase(dev, effect_id);
		if (ret) {
			pr_err("%s error. ret=%d\n", __func__, ret);
			goto err;
		}
	}

	ddata->compose.upload_partial_effect = 0;
err:
	return ret;
}

static void compose_effects_play_work(struct kthread_work *work)
{
	struct sec_vib_inputff_compose *compose =
	    container_of(work, struct sec_vib_inputff_compose, kwork);
	struct sec_vib_inputff_drvdata *ddata =
	    container_of(compose, struct sec_vib_inputff_drvdata, compose);
	struct input_dev *dev = ddata->input;
	struct ff_effect *effect = NULL;
	u16 gain = 0;
	int ret = 0, type = 0, duration = 0;

	pr_info("%s start\n", __func__);

	if (!dev) {
		ret = -ENOENT;
		pr_err("%s ddata->input null\n", __func__);
		goto exit;
	}

	if (compose->pattern_idx < compose->num_of_compose_effects) {
		gain = ddata->effects[compose->pattern_idx].gain;
		effect = &ddata->effects[compose->pattern_idx].compose_effects;
		type = ddata->effects[compose->pattern_idx].compose_effects.type;
		duration = effect->replay.length;

		if (compose->effect_state == COMPOSE_EFFECT_STOP) {
			compose_effect_erase(dev, compose->curr_effect.id);
			compose->effect_state = COMPOSE_EFFECT_END;
		}
		
		switch (type) {
		case FF_CONSTANT:
		case FF_PERIODIC:
			pr_info("%s type %s duration %dms\n", __func__,
				(type == FF_CONSTANT) ? "FF_CONSTANT" : "FF_PERIODIC", duration);

			compose_effect_set_gain(dev, gain);

			if (compose_effect_upload(dev, effect))
				goto exit;
			compose->curr_effect = *effect;
			compose->effect_state = COMPOSE_EFFECT_PLAY;
			compose_effect_playback_on(dev, effect);
			break;
		case VIB_FREE_DURATION:
			pr_info("%s delay duration %dms\n", __func__, duration);
			break;
		default:
			pr_err("%s error. type=%d\n", __func__, type);
			break;
		}

		hrtimer_start(&ddata->compose_effects_timer,
			ktime_set(duration / 1000,(duration % 1000) * 1000000),
			HRTIMER_MODE_REL);

		compose->pattern_idx++;

		if (compose->pattern_idx >= ddata->compose.num_of_compose_effects && 
			ddata->compose.compose_repeat)
			compose->pattern_idx = 0;
	}

exit:
	pr_info("%s exit\n", __func__);
}

static void stop_compose_effects_thread(struct sec_vib_inputff_drvdata *ddata)
{
	pr_info("%s\n", __func__);

	if (!ddata)
		return;

	hrtimer_cancel(&ddata->compose_effects_timer);
	if (ddata->compose.effect_state == COMPOSE_EFFECT_PLAY) {
		compose_effect_playback_off(ddata->input, &(ddata->compose.curr_effect));
		ddata->compose.effect_state = COMPOSE_EFFECT_STOP;
	}
}

static enum hrtimer_restart compose_effect_timer(struct hrtimer *timer)
{
	struct sec_vib_inputff_drvdata *ddata =
		container_of(timer, struct sec_vib_inputff_drvdata, compose_effects_timer);

	if (ddata->compose.effect_state == COMPOSE_EFFECT_PLAY) {
		compose_effect_playback_off(ddata->input, &(ddata->compose.curr_effect));
		ddata->compose.effect_state = COMPOSE_EFFECT_STOP;
	}
	kthread_queue_work(&ddata->compose.kworker, &ddata->compose.kwork);
	return HRTIMER_NORESTART;
}

__visible_for_testing bool check_common_inputff_compose_pattern(struct sec_vib_inputff_drvdata *ddata,
		struct ff_effect *effect)
{
	if (!ddata || !effect)
		return false;

	if (ddata->use_common_inputff) {
		if (effect->u.periodic.custom_len > sizeof(int))
			return true;
	}
	return false;
}

__visible_for_testing bool uploaded_common_inputff_compose_pattern(struct sec_vib_inputff_drvdata *ddata)
{
	if (!ddata)
		return false;

	if (ddata->use_common_inputff) {
		if (ddata->compose.upload_compose_effect)
			return true;
	}
	return false;
}

static int process_common_inputff_compose_upload_effect(struct sec_vib_inputff_drvdata *ddata,
		struct ff_effect *effect)
{
	struct common_inputff_effects input_compose_effects;
	int ret = 0;

	if (!ddata || !effect) {
		ret = -ENOENT;
		pr_err("%s ddata or effect null\n", __func__);
		goto err;
	}

	if (copy_from_user(&input_compose_effects,
			effect->u.periodic.custom_data,
			sizeof(struct common_inputff_effects))) {
		ret = -ENODATA;
		goto err;
	}
	ddata->compose.compose_effect_id = effect->id;
	ret = parsing_compose_effects(ddata, &input_compose_effects);
	if (ret)
		goto err;
	ddata->compose.upload_compose_effect = 1;
err:
	return ret;
}

static void process_common_inputff_compose_erase(struct sec_vib_inputff_drvdata *ddata)
{
	if (!ddata)
		return;

	stop_compose_effects_thread(ddata);
	kthread_flush_work(&ddata->compose.kwork);
	if (ddata->compose.effect_state == COMPOSE_EFFECT_STOP) {
		compose_effect_erase(ddata->input, ddata->compose.curr_effect.id);
		ddata->compose.effect_state = COMPOSE_EFFECT_END;
	}
	ddata->compose.upload_compose_effect = 0;
	ddata->compose.num_of_compose_effects = 0;
	ddata->compose.compose_repeat = 0;
	ddata->compose.compose_effect_id = -1;
}

static void process_common_inputff_compose_playback(struct sec_vib_inputff_drvdata *ddata,
		int val)
{
	if (!ddata)
		return;

	if (val) {
		kthread_queue_work(&ddata->compose.kworker, &ddata->compose.kwork);
	} else
		stop_compose_effects_thread(ddata);
}

static int sec_vib_inputff_upload_effect(struct input_dev *dev,
					 struct ff_effect *effect,
					 struct ff_effect *old)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	if (!ddata) {
		ret = -ENOENT;
		pr_err("%s ddata null\n", __func__);
		goto err;
	}

	if (check_common_inputff_compose_pattern(ddata, effect)) {
		ret = process_common_inputff_compose_upload_effect(ddata, effect);
	} else {
		if (ddata->vib_ops->upload)
			ret = ddata->vib_ops->upload(dev, effect, old);
	}
err:
	return ret;
}

static int sec_vib_inputff_erase(struct input_dev *dev, int effect_id)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	if (!ddata) {
		ret = -ENOENT;
		pr_err("%s ddata null\n", __func__);
		goto exit;
	}

	if (uploaded_common_inputff_compose_pattern(ddata)) {
		process_common_inputff_compose_erase(ddata);
		if (ddata->compose.upload_partial_effect == 0)
			goto exit;
	}

	if (ddata->vib_ops->erase)
		ret = ddata->vib_ops->erase(dev, effect_id);

exit:
	return ret;
}

static int sec_vib_inputff_playback(struct input_dev *dev, int effect_id,
				    int val)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);
	int ret = 0;

	if (!ddata) {
		ret = -ENOENT;
		pr_err("%s ddata null\n", __func__);
		goto err;
	}

	if (uploaded_common_inputff_compose_pattern(ddata)) {
		process_common_inputff_compose_playback(ddata, val);
	} else {
		if (ddata->vib_ops->playback)
			ret = ddata->vib_ops->playback(dev, effect_id, val);
	}
err:
	return ret;
}

static void sec_vib_inputff_set_gain(struct input_dev *dev, u16 gain)
{
	struct sec_vib_inputff_drvdata *ddata = input_get_drvdata(dev);

	if (!ddata) {
		pr_err("%s ddata null\n", __func__);
		goto err;
	}

	ddata->effect_gain = gain;
	if (ddata->vib_ops->set_gain)
		ddata->vib_ops->set_gain(dev, gain);
err:
	return;
}

int sec_vib_inputff_register(struct sec_vib_inputff_drvdata *ddata)
{
	int i, ret = 0, ff = 0, ff_cnt = 0;
	struct device *dev = NULL;

	if (!ddata) {
		ret = -ENODEV;
		pr_err("%s no ddata\n", __func__);
		goto err_nodev;
	}
	dev = ddata->dev;

	pr_info("%s +++\n", __func__);

	ddata->input = devm_input_allocate_device(dev);
	if (!ddata->input) {
		ret = -ENOMEM;
		dev_err(dev, "%s no memory\n", __func__);
		goto err_nomem;
	}

	ddata->input->name = "sec_vibrator_inputff";
	ddata->input->id.product = ddata->devid;
	ddata->input->id.version = ddata->revid;

	input_set_drvdata(ddata->input, ddata);

	init_waitqueue_head(&ddata->compose.delay_wait);
	kthread_init_worker(&ddata->compose.kworker);
	kthread_init_work(&ddata->compose.kwork, compose_effects_play_work);

	ddata->compose.compose_thread = kthread_run(kthread_worker_fn,
			&ddata->compose.kworker, "compose-effects");
	if (IS_ERR(ddata->compose.compose_thread)) {
		pr_err("%s Unable to start compose effects thread\n", __func__);
		ret = PTR_ERR(ddata->compose.compose_thread);
		goto err_nomem;
	}

	for (ff = FF_EFFECT_MIN; ff <= FF_MAX_EFFECTS; ff++) {
		if (sec_vib_inputff_getbit(ddata, ff)) {
			input_set_capability(ddata->input, EV_FF, ff);
			ff_cnt++;
		}
	}

	ret = input_ff_create(ddata->input, ff_cnt);
	if (ret) {
		pr_err("Failed to create FF device: %d\n", ret);
		goto err_input_ff;
	}

	/*
	 * input_ff_create() automatically sets FF_RUMBLE capabilities;
	 * we want to restrtict this to only FF_PERIODIC
	 */
	__clear_bit(FF_RUMBLE, ddata->input->ffbit);

	ddata->input->ff->upload = sec_vib_inputff_upload_effect;
	ddata->input->ff->erase = sec_vib_inputff_erase;
	ddata->input->ff->playback = sec_vib_inputff_playback;
	ddata->input->ff->set_gain = sec_vib_inputff_set_gain;

	ret = input_register_device(ddata->input);
	if (ret) {
		pr_err("Cannot register input device: %d\n", ret);
		goto err_input_reg;
	}

	for (i = 0; ddata->vendor_dev_attr_groups[i]; i++) {
		ret = sysfs_create_group(&ddata->input->dev.kobj,
			ddata->vendor_dev_attr_groups[i]);
		if (ret) {
			pr_err("Failed to create sysfs groups[%d] : ret - %d\n", i, ret);
			goto err_sysfs;
		}
	}
	ret = sec_vib_inputff_sysfs_init(ddata);
	if (ret) {
		pr_err("Failed to create inputff sysfs group: %d\n", ret);
		goto err_sysfs;
	}

	ddata->pdata = &sec_vib_inputff_pdata;

#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
	sec_vib_inputff_event_cmd(ddata);
#endif

	hrtimer_init(&ddata->compose_effects_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->compose_effects_timer.function = compose_effect_timer;

	ddata->vibe_init_success = true;

	pr_info("%s ---\n", __func__);
	return ret;
err_sysfs:
	if (ddata->input)
		input_unregister_device(ddata->input);
err_input_reg:
err_input_ff:
	kthread_stop(ddata->compose.compose_thread);
err_nomem:
err_nodev:
	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_register);

void sec_vib_inputff_unregister(struct sec_vib_inputff_drvdata *ddata)
{
	struct device *dev = NULL;

	if (!ddata) {
		pr_err("%s no ddata\n", __func__);
		goto fail;
	}
	dev = ddata->dev;

	pr_info("%s +++\n", __func__);

	if (ddata->vibe_init_success) {
		int i;
		sec_vib_inputff_sysfs_exit(ddata);
		for (i = 0; ddata->vendor_dev_attr_groups[i]; i++) {
			sysfs_remove_group(&ddata->input->dev.kobj,
				ddata->vendor_dev_attr_groups[i]);
		}
	}
	if (ddata->input)
		input_unregister_device(ddata->input);

	if (ddata->compose.compose_thread)
		kthread_stop(ddata->compose.compose_thread);

	ddata->vibe_init_success = false;
	hrtimer_cancel(&ddata->compose_effects_timer);

fail:
	return;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_unregister);

/*
 * sec_vib_inputff_recheck_gain() - facilitate drvier IC to finds
 *		the appropriate gain ratio by checking temperature
 *		& fold position.
 * @ddata - driver data passed from the driver ic.
 */
int sec_vib_inputff_recheck_gain(struct sec_vib_inputff_drvdata *ddata)
{
	int temp = sec_vib_inputff_get_current_temp(ddata);

	if (temp >= ddata->pdata->high_temp_ref)
		return ddata->pdata->high_temp_ratio;
#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
	else
		return set_fold_model_ratio(ddata);
#endif

	return ddata->pdata->normal_ratio;
}

/*
 * sec_vib_inputff_tune_gain() - facilitate drvier IC to finds
 *		the appropriate gain ratio by checking temperature
 *		& fold position.
 * @ddata - driver data passed from the driver ic.
 * @gain - Currently selected gain value
 *
 * return : Tuned gain value based on temperature, fold state, etc.,
 */
int sec_vib_inputff_tune_gain(struct sec_vib_inputff_drvdata *ddata, int gain)
{
	int ratio = sec_vib_inputff_recheck_gain(ddata);
	int tuned_gain = (gain * ratio) / 100;

	pr_info("%s: gain: %d, ratio: %d, tunned gain: %d\n",
			__func__, gain, ratio, tuned_gain);

	return tuned_gain;
}
EXPORT_SYMBOL_GPL(sec_vib_inputff_tune_gain);

/*
 * sec_vib_inputff_parse_dt - parses hw details from dts
 */
static int sec_vib_inputff_parse_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int temp;

	pr_info("%s +++\n", __func__);

	if (unlikely(!np)) {
		pr_err("err: could not parse dt node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "haptic,normal_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! normal_ratio not found\n", __func__);
		sec_vib_inputff_pdata.normal_ratio = 100;  // 100%
	} else
		sec_vib_inputff_pdata.normal_ratio = (int)temp;

	ret = of_property_read_u32(np, "haptic,overdrive_ratio", &temp);
	if (ret) {
		pr_info("%s: overdrive_ratio isn't used\n", __func__);
		sec_vib_inputff_pdata.overdrive_ratio = sec_vib_inputff_pdata.normal_ratio;
	} else
		sec_vib_inputff_pdata.overdrive_ratio = (int)temp;

	ret = of_property_read_u32(np, "haptic,high_temp_ref", &temp);
	if (ret) {
		pr_info("%s: high temperature concept isn't used\n", __func__);
		sec_vib_inputff_pdata.high_temp_ref = SEC_VIBRATOR_INPUTFF_DEFAULT_HIGH_TEMP_REF;
	} else {
		sec_vib_inputff_pdata.high_temp_ref = (int)temp;

		ret = of_property_read_u32(np, "haptic,high_temp_ratio", &temp);
		if (ret) {
			pr_info("%s: high_temp_ratio isn't used\n", __func__);
			sec_vib_inputff_pdata.high_temp_ratio = SEC_VIBRATOR_INPUTFF_DEFAULT_HIGH_TEMP_RATIO;
		}
		else
			sec_vib_inputff_pdata.high_temp_ratio = (int)temp;
	}

#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
	ret = of_property_read_string(np, "haptic,fold_string",
		(const char **)&sec_vib_inputff_pdata.fold_cmd);
	if (ret < 0)
		pr_err("%s: WARNING! fold_string not found\n", __func__);

	ret = of_property_read_u32(np,
			"haptic,tent_open_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! tent_open_ratio not found\n", __func__);
		sec_vib_inputff_pdata.tent_open_ratio = sec_vib_inputff_pdata.normal_ratio;
	} else
		sec_vib_inputff_pdata.tent_open_ratio = (int)temp;

	ret = of_property_read_u32(np,
			"haptic,tent_close_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! tent_close_ratio not found\n", __func__);
		sec_vib_inputff_pdata.tent_close_ratio = sec_vib_inputff_pdata.normal_ratio;
	} else
		sec_vib_inputff_pdata.tent_close_ratio = (int)temp;

	ret = of_property_read_u32(np, "haptic,fold_open_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! fold_open_ratio not found\n", __func__);
		sec_vib_inputff_pdata.fold_open_ratio = sec_vib_inputff_pdata.normal_ratio;
	} else
		sec_vib_inputff_pdata.fold_open_ratio = (int)temp;

	ret = of_property_read_u32(np,
			"haptic,fold_close_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! fold_close_ratio not found\n", __func__);
		sec_vib_inputff_pdata.fold_close_ratio = sec_vib_inputff_pdata.normal_ratio;
	} else
		sec_vib_inputff_pdata.fold_close_ratio = (int)temp;
#endif

	ret = of_property_read_string(np, "haptic,f0_cal_way",
		(const char **)&sec_vib_inputff_pdata.f0_cal_way);
	if (ret < 0) {
		sec_vib_inputff_pdata.f0_cal_way = "NONE";
		pr_err("%s: WARNING! F0 Calibration Method not found\n", __func__);
	}

	pr_info("%s : done! ---\n", __func__);
	return 0;
}

static int sec_vib_inputff_probe(struct platform_device *pdev)
{
	pr_info("%s +++\n", __func__);

	if (unlikely(sec_vib_inputff_parse_dt(pdev)))
		pr_err("%s: WARNING!>..parse dt failed\n", __func__);

	sec_vib_inputff_pdata.probe_done = true;
	pr_info("%s : done! ---\n", __func__);

	return 0;
}


static const struct of_device_id sec_vib_inputff_id[] = {
	{ .compatible = "sec_vib_inputff" },
	{ }
};

static struct platform_driver sec_vib_inputff_driver = {
	.probe		= sec_vib_inputff_probe,
	.driver		= {
		.name	=  "sec_vib_inputff",
		.owner	= THIS_MODULE,
		.of_match_table = sec_vib_inputff_id,
	},
};

module_platform_driver(sec_vib_inputff_driver);

MODULE_AUTHOR("Samsung electronics <wookwang.lee@samsung.com>, Samsung vibrator team members");
MODULE_DESCRIPTION("sec_vibrator_inputff");
MODULE_LICENSE("GPL");
