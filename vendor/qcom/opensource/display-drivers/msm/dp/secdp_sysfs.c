// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <drm/drm_edid.h>
#include <linux/string.h>
#if defined(CONFIG_SECDP_BIGDATA)
#include <linux/secdp_bigdata.h>
#endif

#include "dp_link.h"
#include "dp_panel.h"
#include "dp_power.h"
#include "dp_catalog.h"
#include "dp_debug.h"
#include "dp_display.h"
#include "secdp.h"
#include "secdp_sysfs.h"
#include "sde_edid_parser.h"
#include "secdp_unit_test.h"

enum secdp_unit_test_cmd {
	SECDP_UTCMD_EDID_PARSE = 0,
};

struct secdp_sysfs_private {
	struct device *dev;
	struct dp_parser *parser;
	struct dp_panel *panel;
	struct dp_power *power;
	struct dp_link  *link;
	struct dp_ctrl  *ctrl;
	struct dp_catalog *catalog;
	struct secdp_misc *sec;
	struct secdp_sysfs dp_sysfs;
	enum secdp_unit_test_cmd test_cmd;
};

static inline char *secdp_utcmd_to_str(u32 cmd_type)
{
	switch (cmd_type) {
	case SECDP_UTCMD_EDID_PARSE:
		return SECDP_ENUM_STR(SECDP_UTCMD_EDID_PARSE);
	default:
		return "unknown";
	}
}

/** check if buf has range('-') format
 * @buf		buf to be checked
 * @size	buf size
 * @retval	0 if args are ok, -1 if '-' included
 */
static int secdp_check_store_args(const char *buf, size_t size)
{
	int ret;

	if (strnchr(buf, size, '-')) {
		DP_ERR("range is forbidden!\n");
		ret = -1;
		goto exit;
	}

	ret = 0;
exit:
	return ret;
}

static struct secdp_sysfs_private *secdp_get_sysfs_private(const struct class *class)
{
	struct secdp_sysfs *dp_sysfs;

	dp_sysfs = container_of(class, struct secdp_sysfs, dp_class);
	return container_of(dp_sysfs, struct secdp_sysfs_private, dp_sysfs);
}

#if defined(CONFIG_SECDP_FACTORY_DPSWITCH_TEST)
static ssize_t dp_sbu_sw_sel_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	int sbu_sw_sel, sbu_sw_oe, val[10] = {0,};

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	get_options(buf, ARRAY_SIZE(val), val);
	sbu_sw_sel = val[1];
	sbu_sw_oe = val[2];
	DP_INFO("sw_sel:%d sw_oe:%d\n", sbu_sw_sel, sbu_sw_oe);

	sysfs = secdp_get_sysfs_private(class);

	if (sbu_sw_oe == 0/*on*/)
		secdp_config_gpios_factory(sysfs->power, sbu_sw_sel, true);
	else if (sbu_sw_oe == 1/*off*/)
		secdp_config_gpios_factory(sysfs->power, sbu_sw_sel, false);
	else
		DP_ERR("unknown sw_oe %d\n", sbu_sw_oe);

exit:
	return size;
}

static CLASS_ATTR_WO(dp_sbu_sw_sel);
#endif

#define SECDP_DEX_ADAPTER_SKIP	"SkipAdapterCheck"
#define SECDP_EXTDISP_OFF	"ExtDispOff"

static ssize_t dex_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	struct secdp_dex *dex;
	int rc = 0;

	sysfs = secdp_get_sysfs_private(class);
	sec = sysfs->sec;
	dex = &sec->dex;

	if (!secdp_get_cable_status(sysfs->dev) ||
			!secdp_get_hpd_status(sysfs->dev) ||
			secdp_get_poor_connection_status(sysfs->link) ||
			!secdp_get_link_train_status(sysfs->ctrl)) {
		DP_INFO("cable is out\n");
		dex->prev = dex->curr = dex->status = DEX_DISABLED;
	}

	DP_INFO("prev:%d curr:%d status:%d %s:%d %s:%d\n",
			dex->prev, dex->curr, dex->status,
			SECDP_DEX_ADAPTER_SKIP, dex->adapter_check_skip,
			SECDP_EXTDISP_OFF, sec->extdisp_off);
#if 1//org
	rc = scnprintf(buf, PAGE_SIZE, "%d\n", dex->status);
#else//.TODO: DeX team needs to check if this chnage is good to parse
	rc = scnprintf(buf, PAGE_SIZE, "%d,%s:%d,%s:%d\n", dex->status,
			SECDP_DEX_ADAPTER_SKIP, dex->adapter_check_skip,
			SECDP_EXTDISP_OFF, sec->extdisp_off);
#endif

	if (dex->status == DEX_MODE_CHANGING)
		dex->status = dex->curr;

	return rc;
}

static int dex_store_hmd(struct secdp_sysfs_private *sysfs, char *str,
		int len, size_t size, char *p)
{
	struct secdp_misc *sec = sysfs->sec;
	int num_hmd = 0, sz = 0, ret = 0;
	char *tok;

	mutex_lock(&sec->hmd.lock);

	tok = strsep(&p, ",");
	if (!tok) {
		DP_ERR("%s wrong input!\n", DEX_TAG_HMD);
		ret = -EINVAL;
		goto exit;
	}

	sz  = strlen(tok);
	ret = kstrtouint(tok, 10, &num_hmd);
	if (ret) {
		DP_ERR("%s error %d\n", DEX_TAG_HMD, ret);
		goto exit;
	}

	DP_INFO("%s num:%d sz:%d\n", DEX_TAG_HMD, num_hmd, sz);

	ret = secdp_store_hmd_dev(sysfs->sec, str + (len + sz + 2),
			size - (len + sz + 2), num_hmd);
exit:
	mutex_unlock(&sec->hmd.lock);
	return ret;
}

static int dex_store_adapter_skip(struct secdp_sysfs_private *sysfs, char *p)
{
	struct secdp_misc *sec = sysfs->sec;
	struct secdp_dex *dex = &sec->dex;
	int param = 0, sz = 0, ret = 0;
	char *tok;

	tok = strsep(&p, ",");
	if (!tok) {
		DP_ERR("%s wrong input!\n", SECDP_DEX_ADAPTER_SKIP);
		return -EINVAL;
	}

	sz  = strlen(tok);
	ret = kstrtouint(tok, 2, &param);

	if (ret) {
		DP_ERR("%s error %d\n", SECDP_DEX_ADAPTER_SKIP, ret);
		return ret;
	}

	DP_INFO("%s param:%d sz:%d\n", SECDP_DEX_ADAPTER_SKIP, param, sz);
	dex->adapter_check_skip = (!param) ? false : true;

	return 0;
}

static int dex_store_extdisp_off(struct secdp_sysfs_private *sysfs, char *p)
{
	struct secdp_misc *sec = sysfs->sec;
	int param = 0, sz = 0, ret = 0;
	char *tok;

	tok = strsep(&p, ",");
	if (!tok) {
		DP_ERR("%s wrong input!\n", SECDP_EXTDISP_OFF);
		return -EINVAL;
	}

	sz  = strlen(tok);
	ret = kstrtouint(tok, 2, &param);
	if (ret) {
		DP_ERR("%s error %d\n", SECDP_EXTDISP_OFF, ret);
		return ret;
	}

	DP_INFO("%s param:%d sz:%d\n", SECDP_EXTDISP_OFF, param, sz);

	sec->extdisp_off = (!param) ? false : true;
	if (sec->extdisp_off)
		secdp_extdisp_off(sec);
	else
		secdp_extdisp_on(sec);

	return 0;
}

/*
 * assume that 1 HMD device has name(14),vid(4),pid(4) each, then
 * max 32 HMD devices(name,vid,pid) need 806 bytes including TAG, NUM, comba
 */
#define MAX_DEX_STORE_LEN	1024

static ssize_t dex_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	struct secdp_dex *dex;
	struct secdp_pdic_noti *pdic_noti;
	char *p, *tok, str[MAX_DEX_STORE_LEN] = {0,};
	int len, val[4] = {0,};
	int setting_ui;	/* setting has Dex mode? if yes, 1. otherwise 0 */
	int run;	/* dex is running now?   if yes, 1. otherwise 0 */

	sysfs = secdp_get_sysfs_private(class);
	sec = sysfs->sec;
	dex = &sec->dex;
	pdic_noti = &sec->pdic_noti;

	if (secdp_get_lpm_mode(sec)) {
		DP_INFO("LPM mode, skip\n");
		goto exit;
	}

	if (size >= MAX_DEX_STORE_LEN) {
		DP_ERR("too long args! %lu\n", size);
		goto exit;
	}

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	memcpy(str, buf, size);
	p   = str;
	tok = strsep(&p, ",");
	len = strlen(tok);
	//DP_DEBUG("tok:%s len:%d\n", tok, len);

	if (len && !strncmp(DEX_TAG_HMD, tok, len)) {
		dex_store_hmd(sysfs, str, len, size, p);
		goto exit;
	}

	if (len && !strncmp(SECDP_DEX_ADAPTER_SKIP, tok, len)) {
		dex_store_adapter_skip(sysfs, p);
		goto exit;
	}

	if (len && !strncmp(SECDP_EXTDISP_OFF, tok, len)) {
		dex_store_extdisp_off(sysfs, p);
		goto exit;
	}

	get_options(buf, ARRAY_SIZE(val), val);
	setting_ui = (val[1] & 0xf0) >> 4;
	run = (val[1] & 0x0f);

	DP_INFO("0x%02x dex.ui:%d run:%d cable:%d\n", val[1],
		setting_ui, run, sec->cable_connected);

	dex->setting_ui = setting_ui;
	dex->status = dex->curr = run;

	mutex_lock(&sec->notifier_lock);
	if (!pdic_noti->registered) {
		int rc;

		/* cancel immediately */
		rc = cancel_delayed_work(&pdic_noti->reg_work);
		DP_DEBUG("notifier get registered by dex, cancel:%d\n", rc);
		destroy_delayed_work_on_stack(&pdic_noti->reg_work);

		/* register */
		rc = secdp_pdic_noti_register_ex(sec, false);
		if (rc)
			DP_ERR("noti register fail %d\n", rc);

		mutex_unlock(&sec->notifier_lock);
		goto exit;
	}
	mutex_unlock(&sec->notifier_lock);

	if (!secdp_get_cable_status(sysfs->dev) ||
			!secdp_get_hpd_status(sysfs->dev) ||
			secdp_get_poor_connection_status(sysfs->link) ||
			!secdp_get_link_train_status(sysfs->ctrl)) {
		DP_INFO("cable is out\n");
		dex->prev = dex->curr = dex->status = DEX_DISABLED;
		goto exit;
	}

	if (sec->hpd.noti_deferred) {
		secdp_send_deferred_hpd_noti(sec);
		dex->prev = dex->setting_ui;
		goto exit;
	}

	if (dex->curr == dex->prev) {
		DP_INFO("dex is already %s\n",
			(dex->curr == DEX_ENABLED) ? "enabled" :
			((dex->curr == DEX_DISABLED) ? "disabled" : "changing"));
		goto exit;
	}

	if (dex->curr != dex->setting_ui) {
		DP_INFO("curr and dex.ui are different %d %d\n",
			dex->curr, dex->setting_ui);
		goto exit;
	}

#if defined(CONFIG_SECDP_BIGDATA)
	if (run)
		secdp_bigdata_save_item(BD_DP_MODE, "DEX");
	else
		secdp_bigdata_save_item(BD_DP_MODE, "MIRROR");
#endif

	if (sec->dex.res == DEX_RES_NOT_SUPPORT) {
		DP_DEBUG("this dongle does not support dex\n");
		goto exit;
	}

	if (!secdp_check_reconnect(sysfs->sec)) {
		DP_INFO("not need reconnect\n");
		goto exit;
	}

	secdp_reconnect(sysfs->sec);
	dex->prev = run;
exit:
	return size;
}

static CLASS_ATTR_RW(dex);

static ssize_t dex_ver_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_adapter *adapter;
	int rc;

	sysfs = secdp_get_sysfs_private(class);
	adapter = &sysfs->sec->adapter;

	DP_INFO("branch revision: HW(0x%X),SW(0x%X,0x%X)\n",
		adapter->fw_ver[0], adapter->fw_ver[1], adapter->fw_ver[2]);

	rc = scnprintf(buf, PAGE_SIZE, "%02X%02X\n",
		adapter->fw_ver[1], adapter->fw_ver[2]);

	return rc;
}

static CLASS_ATTR_RO(dex_ver);

/* note: needs test once wifi is fixed */
static ssize_t monitor_info_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	struct dp_panel *panel;
	struct sde_edid_ctrl *edid_ctrl = NULL;
	struct edid *edid = NULL;
	short prod_id = 0;
	int rc = 0;

	sysfs = secdp_get_sysfs_private(class);
	panel = sysfs->panel;

	edid_ctrl = panel->edid_ctrl;
	if (!edid_ctrl) {
		DP_ERR("unable to find edid_ctrl\n");
		goto exit;
	}

	edid = edid_ctrl->edid;
	if (!edid) {
		DP_ERR("unable to find edid\n");
		goto exit;
	}

	prod_id |= (edid->prod_code[0] << 8) | (edid->prod_code[1]);
	DP_DEBUG("prod_code[0]:%02x, prod_code[1]:%02x, prod_id:%04x\n",
		edid->prod_code[0], edid->prod_code[1], prod_id);

	rc = snprintf(buf, SZ_32, "%s,0x%x,0x%x\n",
			edid_ctrl->vendor_id, prod_id, edid->serial); /* byte order? */
exit:
	return rc;
}

static CLASS_ATTR_RO(monitor_info);

#ifdef SYSFS_BW_CODE
static ssize_t bw_code_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	struct dp_parser *parser;
	struct dp_link *link;
	struct dp_link_params *params;
	int val = 0, rc = 0;

	DP_ENTER("\n");

	sysfs = secdp_get_sysfs_private(class);
	parser = sysfs->parser;
	if (!parser->rf_tx_backoff) {
		DP_DEBUG("RF TX backoff not supported\n");
		goto exit;
	}

	if (!secdp_get_cable_status(sysfs->dev))
		goto exit;

	if (!secdp_get_hpd_status(sysfs->dev))
		goto exit;

	link = sysfs->link;
	params = &link->link_params;
	switch (params->bw_code) {
	case DP_LINK_BW_1_62:
		val = 1;
		break;
	case DP_LINK_BW_2_7:
		val = 2;
		break;
	case DP_LINK_BW_5_4:
		val = 4;
		break;
	case DP_LINK_BW_8_1:
		val = 8;
		break;
	default:
		DP_INFO("unknown bw_code %d\n", params->bw_code);
		val = 0;
		break;
	}

	DP_INFO("bw %d\n", val);
exit:
	rc = snprintf(buf, SZ_32, "%d\n", val);
	return rc;
}
static CLASS_ATTR_RO(bw_code);
#endif

#if defined(CONFIG_SECDP_BIGDATA)
static ssize_t dp_error_info_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	return _secdp_bigdata_show(class, attr, buf);
}

static ssize_t dp_error_info_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	return _secdp_bigdata_store(class, attr, buf, size);
}

static CLASS_ATTR_RW(dp_error_info);
#endif

#ifdef SECDP_SELF_TEST
static struct secdp_sef_test_item g_self_test[] = {
	{DP_ENUM_STR(ST_CLEAR_CMD), .arg_cnt = 0, .arg_str = "clear all configurations"},
	{DP_ENUM_STR(ST_LANE_CNT), .arg_cnt = 1, .arg_str = "lane_count: 1 = 1 lane, 2 = 2 lane, 4 = 4 lane, 555 = disable"},
	{DP_ENUM_STR(ST_LINK_RATE), .arg_cnt = 1, .arg_str = "link_rate: 1 = 1.62G , 2 = 2.7G, 3 = 5.4G, 555 = disable"},
	{DP_ENUM_STR(ST_CONNECTION_TEST), .arg_cnt = 1, .arg_str = "reconnection time(sec) : range = 5 ~ 50, 555 = disable"},
	{DP_ENUM_STR(ST_HDCP_TEST), .arg_cnt = 1, .arg_str = "hdcp on/off time(sec): range = 5 ~ 50, 555 = disable"},
	{DP_ENUM_STR(ST_PREEM_TUN), .arg_cnt = 16, .arg_str = "pre-emphasis calibration value, 555 = disable"},
	{DP_ENUM_STR(ST_VOLTAGE_TUN), .arg_cnt = 16, .arg_str = "voltage-level calibration value, 555 = disable"},
};

int secdp_self_test_status(int cmd)
{
	if (cmd >= ST_MAX)
		return -EINVAL;

	if (g_self_test[cmd].enabled) {
		DP_INFO("%s: %s\n", g_self_test[cmd].cmd_str,
			g_self_test[cmd].enabled ? "true" : "false");
	}

	return g_self_test[cmd].enabled ? g_self_test[cmd].arg_cnt : -1;
}

int *secdp_self_test_get_arg(int cmd)
{
	return g_self_test[cmd].arg;
}

#if 0
void secdp_self_register_clear_func(int cmd, void (*func)(void))
{
	if (cmd >= ST_MAX)
		return;

	g_self_test[cmd].clear = func;
	DP_INFO("%s: clear func was registered.\n", g_self_test[cmd].cmd_str);
}
#endif

static void secdp_self_test_reconnect_work(struct work_struct *work)
{
	struct delayed_work *dw;
	struct secdp_misc *sec;
	int delay;
	static unsigned long test_cnt;

	dw = to_delayed_work(work);
	sec = container_of(dw, struct secdp_misc, self_test_reconnect_work);

	if (!secdp_get_cable_status(sec->dev) ||
			!secdp_get_hpd_status(sec->dev)) {
		DP_INFO("cable is out\n");
		test_cnt = 0;
		return;
	}

	if (sec->self_test_reconnect_cb)
		sec->self_test_reconnect_cb(sec);

	test_cnt++;
	DP_INFO("test_cnt %lu\n", test_cnt);

	delay = g_self_test[ST_CONNECTION_TEST].arg[0];
	schedule_delayed_work(&sec->self_test_reconnect_work,
		msecs_to_jiffies(delay * 1000));
}

void secdp_self_test_start_reconnect(struct secdp_sysfs *dp_sysfs, void (*func)(struct secdp_misc *sec))
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	int delay;

	delay = g_self_test[ST_CONNECTION_TEST].arg[0];
	if (delay > 50 || delay < 5)
		delay = g_self_test[ST_CONNECTION_TEST].arg[0] = 10;

	DP_INFO("start reconnect test %d\n", delay);

	sysfs = container_of(dp_sysfs, struct secdp_sysfs_private, dp_sysfs);
	sec = sysfs->sec;

	sec->self_test_reconnect_cb = func;
	schedule_delayed_work(&sec->self_test_reconnect_work,
		msecs_to_jiffies(delay * 1000));
}

static void secdp_self_test_hdcp_test_work(struct work_struct *work)
{
	struct delayed_work *dw;
	struct secdp_misc *sec;
	int delay;
	static unsigned long test_cnt;

	dw = to_delayed_work(work);
	sec = container_of(dw, struct secdp_misc, self_test_hdcp_test_work);

	if (!secdp_get_cable_status(sec->dev) ||
			!secdp_get_hpd_status(sec->dev)) {
		DP_INFO("cable is out\n");
		test_cnt = 0;
		return;
	}

	if (sec->self_test_hdcp_off_cb)
		sec->self_test_hdcp_off_cb();

	msleep(3000);

	if (sec->self_test_hdcp_on_cb)
		sec->self_test_hdcp_on_cb();

	test_cnt++;
	DP_INFO("test_cnt %lu\n", test_cnt);

	delay = g_self_test[ST_HDCP_TEST].arg[0];
	schedule_delayed_work(&sec->self_test_hdcp_test_work,
		msecs_to_jiffies(delay * 1000));

}

void secdp_self_test_start_hdcp_test(struct secdp_sysfs *dp_sysfs,
		void (*func_on)(void), void (*func_off)(void))
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	int delay;

	delay = g_self_test[ST_HDCP_TEST].arg[0];
	if (!delay) {
		DP_INFO("hdcp test is aborted\n");
		return;
	}

	if (delay > 50 || delay < 5)
		delay = g_self_test[ST_HDCP_TEST].arg[0] = 10;

	DP_INFO("start hdcp test %d\n", delay);

	sysfs = container_of(dp_sysfs, struct secdp_sysfs_private, dp_sysfs);
	sec = sysfs->sec;

	sec->self_test_hdcp_on_cb = func_on;
	sec->self_test_hdcp_off_cb = func_off;

	schedule_delayed_work(&sec->self_test_hdcp_test_work,
		msecs_to_jiffies(delay * 1000));
}

static ssize_t dp_self_test_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	int i, j, rc = 0;

	for (i = 0; i < ST_MAX; i++) {
		rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"%d. %s: %s\n   ==>", i,
				g_self_test[i].cmd_str, g_self_test[i].arg_str);

		if (g_self_test[i].enabled) {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
					"current value: enabled - arg: ");

			for (j = 0; j < g_self_test[i].arg_cnt; j++) {
				rc += scnprintf(buf + rc, PAGE_SIZE - rc,
						"0x%x ", g_self_test[i].arg[j]);
			}

			rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n\n");
		} else {
			rc += scnprintf(buf + rc, PAGE_SIZE - rc,
				"current value: disabled\n\n");
		}
	}

	return rc;
}

static void dp_self_test_clear_func(int cmd)
{
	int arg_cnt = (g_self_test[cmd].arg_cnt < ST_ARG_CNT) ? g_self_test[cmd].arg_cnt : ST_ARG_CNT;

	g_self_test[cmd].enabled = false;
	memset(g_self_test[cmd].arg, 0,	sizeof(int) * arg_cnt);

	if (g_self_test[cmd].clear)
		g_self_test[cmd].clear();
}

static ssize_t dp_self_test_store(const struct class *dev,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	int val[ST_ARG_CNT + 2] = {0, };
	int arg, arg_cnt, cmd, i;

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto end;
	}

	get_options(buf, ARRAY_SIZE(val), val);
	cmd = val[1];
	arg = val[2];

	if (cmd < 0 || cmd >= ST_MAX) {
		DP_INFO("invalid cmd\n");
		goto end;
	}

	if (cmd == ST_CLEAR_CMD) {
		for (i = 1; i < ST_MAX; i++)
			dp_self_test_clear_func(i);

		DP_INFO("cmd: ST_CLEAR_CMD\n");
		goto end;
	}

	g_self_test[cmd].enabled = (arg == ST_TEST_EXIT) ? false : true;
	if (g_self_test[cmd].enabled) {
		if ((val[0] - 1) != g_self_test[cmd].arg_cnt) {
			DP_INFO("invalid param.\n");
			goto end;
		}

		arg_cnt = (g_self_test[cmd].arg_cnt < ST_ARG_CNT) ? g_self_test[cmd].arg_cnt : ST_ARG_CNT;
		memcpy(g_self_test[cmd].arg, val + 2, sizeof(int) * arg_cnt);
	} else {
		dp_self_test_clear_func(cmd);
	}

	DP_INFO("cmd: %d. %s, enable:%s\n", cmd,
		(cmd < ST_MAX) ? g_self_test[cmd].cmd_str : "null",
		(cmd < ST_MAX) ? (g_self_test[cmd].enabled ? "true" : "false") : "null");
end:
	return size;
}

static CLASS_ATTR_RW(dp_self_test);
#endif

#if defined(CONFIG_SECDP_DBG)
bool secdp_func_trace;
#define SECDP_FUNC_TRACE  "func_trace"   //ex) echo "func_trace,1" > dp_debug
#define SECDP_SSC_ONOFF	  "ssc"          //ex) echo "ssc,1" > dp_debug

static ssize_t dp_debug_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	struct secdp_dex *dex;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);
	sec = sysfs->sec;
	dex = &sec->dex;

	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "==========\n");
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "[dbg] func_trace: %d\n", secdp_func_trace);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "[dex] adapter_check_skip: %d\n", dex->adapter_check_skip);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "[psm] extdisp_off: %d\n", sec->extdisp_off);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "[ssc] ssc_en: %d\n", secdp_debug_get_ssc(sec));

	secdp_show_hmd_dev(sysfs->sec, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< HMD >\n%s\n", tmp);

	memset(tmp, 0, ARRAY_SIZE(tmp));
	secdp_show_phy_param(sysfs->parser, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< DP-PHY >\n%s\n", tmp);

	memset(tmp, 0, ARRAY_SIZE(tmp));
	secdp_show_preshoot_param(sysfs->parser, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "< PRESHOOT >\n%s\n", tmp);

	memset(tmp, 0, ARRAY_SIZE(tmp));
	secdp_show_link_param(sysfs->link, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< link params >\n%s\n", tmp);

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	memset(tmp, 0, ARRAY_SIZE(tmp));
	secdp_show_ps5169_param(sysfs->parser, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< PS5169 EQ0/EQ1 >\n%s\n", tmp);
#endif

	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "==========\n");
	return rc;
}

static ssize_t dp_debug_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	struct secdp_misc *sec;
	char str[MAX_DEX_STORE_LEN] = {0,}, *p, *tok;
	int len, val[10] = {0, };

	if (size >= MAX_DEX_STORE_LEN) {
		DP_ERR("too long args! %lu\n", size);
		goto exit;
	}

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	sysfs = secdp_get_sysfs_private(class);
	sec = sysfs->sec;

	get_options(buf, ARRAY_SIZE(val), val);

	memcpy(str, buf, size);
	p   = str;
	tok = strsep(&p, ",");
	if (!p) {
		DP_ERR("error: no token found\n");
		goto exit;
	}

	len = strlen(tok);
	if (!len) {
		DP_ERR("error: token len is zero\n");
		goto exit;
	}

	DP_DEBUG("tok:%s, len:%d\n", tok, len);

	if (len && !strncmp(SECDP_FUNC_TRACE, tok, len)) {
		int param = 0, sz = 0, ret;

		tok = strsep(&p, ",");
		if (!tok) {
			DP_ERR("wrong input!\n");
			goto exit;
		}
		sz  = strlen(tok);
		ret = kstrtouint(tok, 2, &param);
		if (ret) {
			DP_ERR("error:%d\n", ret);
			goto exit;
		}

		DP_DEBUG("[%s] param:%d sz:%d ret:%d\n", SECDP_FUNC_TRACE,
			param, sz, ret);

		secdp_func_trace = param ? 1 : 0;
		goto exit;
	}

	if (len && !strncmp(SECDP_SSC_ONOFF, tok, len)) {
		int param = 0, sz = 0, ret;

		tok = strsep(&p, ",");
		if (!tok) {
			DP_ERR("wrong input!\n");
			goto exit;
		}
		sz  = strlen(tok);
		ret = kstrtouint(tok, 2, &param);
		if (ret) {
			DP_ERR("error:%d\n", ret);
			goto exit;
		}

		DP_DEBUG("[%s] param:%d sz:%d ret:%d\n", SECDP_SSC_ONOFF,
			param, sz, ret);

		secdp_debug_set_ssc(sec, param ? true : false);
		goto exit;
	}

exit:
	return size;
}

static CLASS_ATTR_RW(dp_debug);

static ssize_t dp_unit_test_show(const struct class *class,
				const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	int rc, cmd;
	bool res = false;

	sysfs = secdp_get_sysfs_private(class);
	cmd = sysfs->test_cmd;
	DP_INFO("test_cmd: %s\n", secdp_utcmd_to_str(cmd));

	switch (cmd) {
	case SECDP_UTCMD_EDID_PARSE:
		res = secdp_unit_test_edid_parse(sysfs->sec);
		break;
	default:
		DP_INFO("invalid test_cmd: %d\n", cmd);
		break;
	}

	rc = scnprintf(buf, 3, "%d\n", res ? 1 : 0);
	return rc;
}

static ssize_t dp_unit_test_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	int val[10] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	sysfs = secdp_get_sysfs_private(class);

	get_options(buf, ARRAY_SIZE(val), val);
	sysfs->test_cmd = val[1];

	DP_INFO("test_cmd: %d...%s\n", sysfs->test_cmd,
		secdp_utcmd_to_str(sysfs->test_cmd));

exit:
	return size;
}

static CLASS_ATTR_RW(dp_unit_test);

static ssize_t dp_aux_cfg_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_aux_cfg_show(sysfs->parser, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "\n< AUX cfg >\n%s\n", tmp);

	return rc;
}

static ssize_t dp_aux_cfg_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, min(ARRAY_SIZE(tmp) - 1, size));
	secdp_aux_cfg_store(sysfs->parser, tmp);

	return size;
}

static CLASS_ATTR_RW(dp_aux_cfg);

static ssize_t dp_hbr2_3_preshoot0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_128] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_preshoot_show(sysfs->parser, DP_LR_HBR2_3, DP_HW_PRESHOOT_0, tmp);
	rc = snprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);
	return rc;
}

static ssize_t dp_hbr2_3_preshoot0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_preshoot_store(sysfs->parser, DP_LR_HBR2_3, DP_HW_PRESHOOT_0, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_hbr2_3_preshoot0);

static ssize_t dp_hbr2_3_preshoot1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_128] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_preshoot_show(sysfs->parser, DP_LR_HBR2_3, DP_HW_PRESHOOT_1, tmp);
	rc = snprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);
	return rc;
}

static ssize_t dp_hbr2_3_preshoot1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_preshoot_store(sysfs->parser, DP_LR_HBR2_3, DP_HW_PRESHOOT_1, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_hbr2_3_preshoot1);

static ssize_t dp_rbr_hbr_preshoot0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_128] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_preshoot_show(sysfs->parser, DP_LR_HBR_RBR, DP_HW_PRESHOOT_0, tmp);
	rc = snprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);
	return rc;
}

static ssize_t dp_rbr_hbr_preshoot0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_preshoot_store(sysfs->parser, DP_LR_HBR_RBR, DP_HW_PRESHOOT_0, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_rbr_hbr_preshoot0);

static ssize_t dp_rbr_hbr_preshoot1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_128] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_preshoot_show(sysfs->parser, DP_LR_HBR_RBR, DP_HW_PRESHOOT_1, tmp);
	rc = snprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);
	return rc;
}

static ssize_t dp_rbr_hbr_preshoot1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_preshoot_store(sysfs->parser, DP_LR_HBR_RBR, DP_HW_PRESHOOT_1, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_rbr_hbr_preshoot1);

static ssize_t dp_hbr2_3_voltage_swing_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_vxpx_show(sysfs->parser, DP_LR_HBR2_3, DP_PARAM_VX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_hbr2_3_voltage_swing_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_vxpx_store(sysfs->parser, DP_LR_HBR2_3, DP_PARAM_VX, tmp);
end:
	return size;

}

static CLASS_ATTR_RW(dp_hbr2_3_voltage_swing);

static ssize_t dp_hbr2_3_pre_emphasis_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_vxpx_show(sysfs->parser, DP_LR_HBR2_3, DP_PARAM_PX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_hbr2_3_pre_emphasis_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_vxpx_store(sysfs->parser, DP_LR_HBR2_3, DP_PARAM_PX, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_hbr2_3_pre_emphasis);

static ssize_t dp_hbr_rbr_voltage_swing_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_vxpx_show(sysfs->parser, DP_LR_HBR_RBR, DP_PARAM_VX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_hbr_rbr_voltage_swing_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_vxpx_store(sysfs->parser, DP_LR_HBR_RBR, DP_PARAM_VX, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_hbr_rbr_voltage_swing);

static ssize_t dp_hbr_rbr_pre_emphasis_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_vxpx_show(sysfs->parser, DP_LR_HBR_RBR, DP_PARAM_PX, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_hbr_rbr_pre_emphasis_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_vxpx_store(sysfs->parser, DP_LR_HBR_RBR, DP_PARAM_PX, tmp);
end:
	return size;
}

static CLASS_ATTR_RW(dp_hbr_rbr_pre_emphasis);

static ssize_t dp_pref_skip_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	int skip, rc;

	DP_ENTER("\n");

	sysfs = secdp_get_sysfs_private(class);
	skip = sysfs->sec->debug.prefer_check_skip;
	rc = snprintf(buf, SZ_8, "%d\n", skip);

	return rc;
}

static ssize_t dp_pref_skip_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	int i, val[30] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	DP_DEBUG("size:%d\n", (int)size);

	get_options(buf, ARRAY_SIZE(val), val);
	for (i = 0; i < 16; i = i + 4) {
		DP_DEBUG("%02x,%02x,%02x,%02x\n",
			val[i+1], val[i+2], val[i+3], val[i+4]);
	}

	sysfs = secdp_get_sysfs_private(class);
	sysfs->sec->debug.prefer_check_skip = val[1];
exit:
	return size;
}

static CLASS_ATTR_RW(dp_pref_skip);

static ssize_t dp_pref_ratio_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	int ratio, rc;

	DP_ENTER("\n");

	sysfs = secdp_get_sysfs_private(class);
	ratio = sysfs->sec->prefer.ratio;
	rc = snprintf(buf, SZ_8, "%d\n", ratio);

	return rc;
}

static ssize_t dp_pref_ratio_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	int i, val[30] = {0, };

	if (secdp_check_store_args(buf, size)) {
		DP_ERR("args error!\n");
		goto exit;
	}

	DP_DEBUG("size:%d\n", (int)size);

	get_options(buf, ARRAY_SIZE(val), val);
	for (i = 0; i < 16; i = i + 4) {
		DP_DEBUG("%02x,%02x,%02x,%02x\n",
			val[i+1], val[i+2], val[i+3], val[i+4]);
	}

	sysfs = secdp_get_sysfs_private(class);
	sysfs->sec->prefer.ratio = val[1];
exit:
	return size;
}

static CLASS_ATTR_RW(dp_pref_ratio);

#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
static ssize_t dp_ps5169_rbr_eq0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_RBR, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_rbr_eq0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_RBR, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_rbr_eq0);

static ssize_t dp_ps5169_rbr_eq1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_RBR, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_rbr_eq1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_RBR, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_rbr_eq1);

static ssize_t dp_ps5169_hbr_eq0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr_eq0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr_eq0);

static ssize_t dp_ps5169_hbr_eq1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr_eq1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr_eq1);

static ssize_t dp_ps5169_hbr2_eq0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR2, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr2_eq0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR2, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr2_eq0);

static ssize_t dp_ps5169_hbr2_eq1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR2, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr2_eq1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR2, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr2_eq1);

static ssize_t dp_ps5169_hbr3_eq0_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR3, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr3_eq0_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ0, DP_PS5169_RATE_HBR3, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr3_eq0);

static ssize_t dp_ps5169_hbr3_eq1_show(const struct class *class,
		const struct class_attribute *attr, char *buf)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_1K] = {0,};
	int  rc = 0;

	sysfs = secdp_get_sysfs_private(class);

	secdp_parse_ps5169_show(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR3, tmp);
	rc += scnprintf(buf + rc, PAGE_SIZE - rc, "%s\n", tmp);

	return rc;
}

static ssize_t dp_ps5169_hbr3_eq1_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size)
{
	struct secdp_sysfs_private *sysfs;
	char tmp[SZ_64] = {0,};
	int len = min(sizeof(tmp), size);

	if (!len || len >= SZ_64) {
		DP_ERR("wrong length! %d\n", len);
		goto end;
	}

	sysfs = secdp_get_sysfs_private(class);

	memcpy(tmp, buf, len);
	tmp[SZ_64 - 1] = '\0';
	secdp_parse_ps5169_store(sysfs->parser, DP_PS5169_EQ1, DP_PS5169_RATE_HBR3, tmp);
end:
	return size;
}
static CLASS_ATTR_RW(dp_ps5169_hbr3_eq1);
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif/*CONFIG_SECDP_DBG*/

enum {
	DEX = 0,
	DEX_VER,
	MONITOR_INFO,
#ifdef SYSFS_BW_CODE
	BW_CODE,
#endif
#if defined(CONFIG_SECDP_BIGDATA)
	DP_ERROR_INFO,
#endif
#if defined(CONFIG_SECDP_FACTORY_DPSWITCH_TEST)
	DP_SBU_SW_SEL,
#endif
#ifdef SECDP_SELF_TEST
	DP_SELF_TEST,
#endif
#if defined(CONFIG_SECDP_DBG)
	DP_DBG,
	DP_UNIT_TEST,
	DP_AUX_CFG,
	DP_HBR2_3_VOLTAGE_SWING,
	DP_HBR2_3_PRE_EMPHASIS,
	DP_HBR_RBR_VOLTAGE_SWING,
	DP_HBR_RBR_PRE_EMPHASIS,
	DP_HBR2_3_PRESHOOT0,
	DP_HBR2_3_PRESHOOT1,
	DP_RBR_HBR_PRESHOOT0,
	DP_RBR_HBR_PRESHOOT1,
	DP_PREF_SKIP,
	DP_PREF_RATIO,
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	DP_PS5169_RBR_EQ0,
	DP_PS5169_RBR_EQ1,
	DP_PS5169_HBR_EQ0,
	DP_PS5169_HBR_EQ1,
	DP_PS5169_HBR2_EQ0,
	DP_PS5169_HBR2_EQ1,
	DP_PS5169_HBR3_EQ0,
	DP_PS5169_HBR3_EQ1,
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif
};

static struct attribute *secdp_class_attrs[] = {
	[DEX]		= &class_attr_dex.attr,
	[DEX_VER]	= &class_attr_dex_ver.attr,
	[MONITOR_INFO]	= &class_attr_monitor_info.attr,
#ifdef SYSFS_BW_CODE
	[BW_CODE]	= &class_attr_bw_code.attr,
#endif
#if defined(CONFIG_SECDP_BIGDATA)
	[DP_ERROR_INFO] = &class_attr_dp_error_info.attr,
#endif
#if defined(CONFIG_SECDP_FACTORY_DPSWITCH_TEST)
	[DP_SBU_SW_SEL]	= &class_attr_dp_sbu_sw_sel.attr,
#endif
#ifdef SECDP_SELF_TEST
	[DP_SELF_TEST]	= &class_attr_dp_self_test.attr,
#endif
#if defined(CONFIG_SECDP_DBG)
	[DP_DBG]	= &class_attr_dp_debug.attr,
	[DP_UNIT_TEST]	= &class_attr_dp_unit_test.attr,
	[DP_AUX_CFG]	= &class_attr_dp_aux_cfg.attr,
	[DP_HBR2_3_VOLTAGE_SWING]  = &class_attr_dp_hbr2_3_voltage_swing.attr,
	[DP_HBR2_3_PRE_EMPHASIS]   = &class_attr_dp_hbr2_3_pre_emphasis.attr,
	[DP_HBR_RBR_VOLTAGE_SWING] = &class_attr_dp_hbr_rbr_voltage_swing.attr,
	[DP_HBR_RBR_PRE_EMPHASIS]  = &class_attr_dp_hbr_rbr_pre_emphasis.attr,
	[DP_HBR2_3_PRESHOOT0] = &class_attr_dp_hbr2_3_preshoot0.attr,
	[DP_HBR2_3_PRESHOOT1] = &class_attr_dp_hbr2_3_preshoot1.attr,
	[DP_RBR_HBR_PRESHOOT0] = &class_attr_dp_rbr_hbr_preshoot0.attr,
	[DP_RBR_HBR_PRESHOOT1] = &class_attr_dp_rbr_hbr_preshoot1.attr,
	[DP_PREF_SKIP]	= &class_attr_dp_pref_skip.attr,
	[DP_PREF_RATIO]	= &class_attr_dp_pref_ratio.attr,
#if IS_ENABLED(CONFIG_COMBO_REDRIVER_PS5169)
	[DP_PS5169_RBR_EQ0]  = &class_attr_dp_ps5169_rbr_eq0.attr,
	[DP_PS5169_RBR_EQ1]  = &class_attr_dp_ps5169_rbr_eq1.attr,
	[DP_PS5169_HBR_EQ0]  = &class_attr_dp_ps5169_hbr_eq0.attr,
	[DP_PS5169_HBR_EQ1]  = &class_attr_dp_ps5169_hbr_eq1.attr,
	[DP_PS5169_HBR2_EQ0] = &class_attr_dp_ps5169_hbr2_eq0.attr,
	[DP_PS5169_HBR2_EQ1] = &class_attr_dp_ps5169_hbr2_eq1.attr,
	[DP_PS5169_HBR3_EQ0] = &class_attr_dp_ps5169_hbr3_eq0.attr,
	[DP_PS5169_HBR3_EQ1] = &class_attr_dp_ps5169_hbr3_eq1.attr,
#endif/*CONFIG_COMBO_REDRIVER_PS5169*/
#endif
	NULL,
};
ATTRIBUTE_GROUPS(secdp_class);

struct secdp_sysfs *secdp_sysfs_get(struct secdp_sysfs_in *in)
{
	struct class *dp_class = NULL;
	struct secdp_sysfs_private *sysfs = NULL;
	struct secdp_sysfs *dp_sysfs;
	int rc = 0;

	if (!in) {
		DP_ERR("invalid input\n");
		rc = -EINVAL;
		goto error;
	}

	sysfs = devm_kzalloc(in->dev, sizeof(*sysfs), GFP_KERNEL);
	if (!sysfs) {
		rc = -EINVAL;
		DP_ERR("alloc failed %d\n", rc);
		goto error;
	}

	sysfs->dev     = in->dev;
	sysfs->parser  = in->parser;
	sysfs->power   = in->power;
	sysfs->panel   = in->panel;
	sysfs->link    = in->link;
	sysfs->ctrl    = in->ctrl;
	sysfs->catalog = in->catalog;
	sysfs->sec     = in->sec;

	dp_sysfs = &sysfs->dp_sysfs;

	dp_class = &dp_sysfs->dp_class;
	dp_class->name = "dp_sec";
	//dp_class->owner = THIS_MODULE;
	dp_class->class_groups = secdp_class_groups;
	rc = class_register(dp_class);
	if (rc) {
		DP_ERR("cannot register dp_class %d\n", rc);
		goto free_class;
	}

#if defined(CONFIG_SECDP_BIGDATA)
	secdp_bigdata_init(dp_class);
#endif
#ifdef SECDP_SELF_TEST
	INIT_DELAYED_WORK(&sysfs->sec->self_test_reconnect_work,
		secdp_self_test_reconnect_work);
	INIT_DELAYED_WORK(&sysfs->sec->self_test_hdcp_test_work,
		secdp_self_test_hdcp_test_work);
#endif
	DP_DEBUG("success\n");
	return dp_sysfs;

free_class:
	devm_kfree(in->dev, sysfs);
error:
	return ERR_PTR(rc);
}

void secdp_sysfs_put(struct device *dev, struct secdp_sysfs *dp_sysfs)
{
	struct secdp_sysfs_private *sysfs;

	if (!dp_sysfs)
		return;

	class_unregister(&dp_sysfs->dp_class);

	sysfs = container_of(dp_sysfs, struct secdp_sysfs_private, dp_sysfs);
	devm_kfree(dev, sysfs);
}
