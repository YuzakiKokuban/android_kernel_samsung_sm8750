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
#ifndef __SEC_VIBRATOR_INPUTFF_H__
#define __SEC_VIBRATOR_INPUTFF_H__

#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <linux/kthread.h>
#include <linux/version.h>

#define VIB_NOTIFIER_ON	(1)
#define FWLOAD_TRY (3)

enum EVENT_CMD {
	EVENT_CMD_NONE = 0,
	EVENT_CMD_FOLDER_CLOSE,
	EVENT_CMD_FOLDER_OPEN,
	EVENT_CMD_ACCESSIBILITY_BOOST_ON,
	EVENT_CMD_ACCESSIBILITY_BOOST_OFF,
	EVENT_CMD_TENT_CLOSE,
	EVENT_CMD_TENT_OPEN,
	EVENT_CMD_MAX,
};

#define MAX_STR_LEN_VIB_TYPE 32
#define MAX_STR_LEN_EVENT_CMD 32
#define MIN_STR_LEN_EVENT_CMD 4

#define MAX_COMPOSE_EFFECT (32)

#define SEC_VIBRATOR_INPUTFF_DEFAULT_HIGH_TEMP_REF INT_MAX
#define SEC_VIBRATOR_INPUTFF_DEFAULT_HIGH_TEMP_RATIO 100

#define VIB_FREE_DURATION 0

enum compose_effect_state {
	COMPOSE_EFFECT_END = 0,
	COMPOSE_EFFECT_STOP = 1,
	COMPOSE_EFFECT_PLAY = 2,
};

struct common_inputff_effect {
	int type;
	int effect_id;
	int scale;
	int duration;
	int frequency;
};

struct common_inputff_effects {
	struct common_inputff_effect effects[MAX_COMPOSE_EFFECT];
	int num_of_effects;
	int repeat;
};

struct sec_vib_inputff_ops {
	int (*upload)(struct input_dev *dev,
		struct ff_effect *effect, struct ff_effect *old);
	int (*erase)(struct input_dev *dev, int effect_id);
	int (*playback)(struct input_dev *dev,
		int effect_id, int val);
	void (*set_gain)(struct input_dev *dev, u16 gain);
	int (*get_i2c_test)(struct input_dev *dev);
	int (*get_i2s_test)(struct input_dev *dev);
	int (*fw_load)(struct input_dev *dev, unsigned int fw_id);
	int (*set_trigger_cal)(struct input_dev *dev, u32 val);
	u32 (*get_f0_measured)(struct input_dev *dev);
	int (*get_f0_offset)(struct input_dev *dev);
	int (*set_f0_offset)(struct input_dev *dev, u32 val);
	u32 (*get_f0_stored)(struct input_dev *dev);
	u32 (*set_f0_stored)(struct input_dev *dev, u32 val);
	int (*set_le_stored)(struct input_dev *dev, u32 val);
	u32 (*get_le_stored)(struct input_dev *dev);
	int (*get_le_est)(struct input_dev *dev, u32 *le);
	int (*set_use_sep_index)(struct input_dev *dev, bool use_sep_index);
	int (*get_lra_resistance)(struct input_dev *dev);
	const char * (*get_owt_lib_compat_version)(struct input_dev *dev);
	const char * (*get_ap_chipset)(struct input_dev *dev);
};

struct sec_vib_inputff_fwdata {
	int id;
	int retry;
	int ret[FWLOAD_TRY];
	int stat;
	struct workqueue_struct *fw_workqueue;
	struct work_struct wk;
	struct delayed_work retry_wk;
	struct delayed_work store_wk;
	struct wakeup_source ws;
	struct mutex stat_lock;
};

struct sec_vib_inputff_compose_effects {
	struct ff_effect compose_effects;
	u16 gain;
};

struct sec_vib_inputff_compose {
	struct task_struct *compose_thread;
	struct kthread_worker kworker;
	struct kthread_work kwork;
	wait_queue_head_t delay_wait;
	int num_of_compose_effects;
	int upload_compose_effect;
	int compose_effect_id;
	int upload_partial_effect;
	u32 pattern_idx;
	struct ff_effect curr_effect;
	int effect_state;
	int compose_repeat;
};

struct sec_vib_inputff_pdata  {
	bool probe_done;
	int normal_ratio;
	int overdrive_ratio;
	int high_temp_ratio;
	int high_temp_ref;
#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
	int fold_open_ratio;
	int fold_close_ratio;
	int tent_open_ratio;
	int tent_close_ratio;
	const char *fold_cmd;
#endif
	const char *f0_cal_way;
};

struct sec_vib_inputff_drvdata {
	struct class *sec_vib_inputff_class;
	struct device *virtual_dev;
	u32 devid : 24;
	u8 revid;
	u64 ff_val;
	bool vibe_init_success;
	struct device *dev;
	struct input_dev *input;
	struct attribute_group **vendor_dev_attr_groups;
	struct attribute_group *sec_vib_attr_group;
	const struct sec_vib_inputff_ops *vib_ops;
	void *private_data;
	int temperature;
	int ach_percent;
	u32 le_stored;
	u32 f0_stored;
	int support_fw;
	int trigger_calibration;
	struct sec_vib_inputff_fwdata fw;
	bool is_f0_tracking;
	bool is_le_support;
	struct sec_vib_inputff_pdata *pdata;

	enum EVENT_CMD event_idx;
	char event_cmd[MAX_STR_LEN_EVENT_CMD + 1];

	bool use_common_inputff;
	u16 effect_gain;
	struct sec_vib_inputff_compose_effects effects[MAX_COMPOSE_EFFECT];
	struct sec_vib_inputff_compose compose;

	struct work_struct cal_work;
	struct workqueue_struct *cal_workqueue;

	bool fw_init_attempted;
	struct hrtimer compose_effects_timer;
};

/* firmware load status. if fail, return err number */
#define FW_LOAD_INIT	    (1<<0)
#define	FW_LOAD_STORE	    (1<<1)
#define FW_LOAD_SUCCESS	    (1<<2)

extern int sec_vib_inputff_notifier_register(struct notifier_block *nb);
extern int sec_vib_inputff_notifier_unregister(struct notifier_block *nb);
extern int sec_vib_inputff_notifier_notify(void);
extern int sec_vib_inputff_setbit(struct sec_vib_inputff_drvdata *ddata, int val);
extern int sec_vib_inputff_register(struct sec_vib_inputff_drvdata *ddata);
extern void sec_vib_inputff_unregister(struct sec_vib_inputff_drvdata *ddata);
extern int sec_vib_inputff_get_current_temp(struct sec_vib_inputff_drvdata *ddata);
extern int sec_vib_inputff_get_ach_percent(struct sec_vib_inputff_drvdata *ddata);
extern int sec_vib_inputff_tune_gain(struct sec_vib_inputff_drvdata *ddata, int gain);
#if defined(CONFIG_SEC_VIB_FOLD_MODEL)
extern void sec_vib_inputff_event_cmd(struct sec_vib_inputff_drvdata *ddata);
extern int set_fold_model_ratio(struct sec_vib_inputff_drvdata *ddata);
#endif
extern int sec_vib_inputff_sysfs_init(struct sec_vib_inputff_drvdata *ddata);
extern void sec_vib_inputff_sysfs_exit(struct sec_vib_inputff_drvdata *ddata);
#endif //__SEC_VIBRATOR_INPUTFF_H__
