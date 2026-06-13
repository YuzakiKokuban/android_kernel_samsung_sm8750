/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
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
 */

#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/battery/sec_pd.h>

#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>

SEC_PD_SINK_STATUS *g_psink_status;
EXPORT_SYMBOL(g_psink_status);
#else
static SEC_PD_SINK_STATUS *g_psink_status;
#endif

#if defined(CONFIG_ARCH_MTK_PROJECT) || IS_ENABLED(CONFIG_SEC_MTK_CHARGER)
struct pdic_notifier_struct pd_noti;
EXPORT_SYMBOL(pd_noti);
#endif

const char* sec_pd_pdo_type_str(int pdo_type)
{
	switch (pdo_type) {
	case FPDO_TYPE:
		return "FIXED";
	case APDO_TYPE:
		return "APDO";
	case VPDO_TYPE:
		return "VPDO";
	default:
		return "UNKNOWN";
	}
}
EXPORT_SYMBOL(sec_pd_pdo_type_str);

int sec_pd_select_pdo(int num)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_select_pdo) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_select_pdo(num);

	return 0;
}
EXPORT_SYMBOL(sec_pd_select_pdo);

int sec_pd_select_pps(int num, int ppsVol, int ppsCur)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_select_pps) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	return g_psink_status->fp_sec_pd_select_pps(num, ppsVol, ppsCur);
}
EXPORT_SYMBOL(sec_pd_select_pps);

int sec_pd_get_current_pdo(unsigned int *pdo)
{
	if (pdo == NULL) {
		pr_err("%s: invalid argument\n", __func__);
		return -1;
	}

	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	*pdo = g_psink_status->current_pdo_num;
	return 0;
}
EXPORT_SYMBOL(sec_pd_get_current_pdo);

int sec_pd_get_selected_pdo(unsigned int *pdo)
{
	if (pdo == NULL) {
		pr_err("%s: invalid argument\n", __func__);
		return -1;
	}

	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	*pdo = g_psink_status->selected_pdo_num;
	return 0;
}
EXPORT_SYMBOL(sec_pd_get_selected_pdo);

int sec_pd_is_apdo(unsigned int pdo)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (pdo > g_psink_status->available_pdo_num) {
		pr_err("%s: invalid argument(%d)\n", __func__, pdo);
		return -EINVAL;
	}

	return ((g_psink_status->power_list[pdo].pdo_type == APDO_TYPE) ? true : false);
}
EXPORT_SYMBOL(sec_pd_is_apdo);

int sec_pd_detach_with_cc(int state)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_detach_with_cc) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_detach_with_cc(state);

	return 0;
}
EXPORT_SYMBOL(sec_pd_detach_with_cc);

static int sec_pd_check_pdo(unsigned int pdo, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	POWER_LIST *pwr = &g_psink_status->power_list[pdo];

	if (pwr->pdo_type == APDO_TYPE) {
		if (min_volt > 0) {
			if (min_volt < pwr->min_voltage)
				return -1;

			if (min_volt > pwr->max_voltage)
				return -1;
		}

		if (max_volt > 0) {
			if (max_volt > pwr->max_voltage)
				return -1;

			if (max_volt < pwr->min_voltage)
				return -1;
		}
	} else {
		if (max_volt != pwr->max_voltage)
			return -1;
	}

	if ((max_curr > 0) && (max_curr > pwr->max_current))
		return -1;

	return sec_pd_get_max_power(pwr->pdo_type,
		pwr->min_voltage, pwr->max_voltage, pwr->max_current);
}

#define PROG_0V 0
#define PROG_5V 5900
#define PROG_9V 11000
#define PROG_15V 16000
#define PROG_20V 21000
#define PROG_MIN 3300
int sec_pd_get_apdo_prog_volt(unsigned int pdo_type, unsigned int max_volt)
{
	if (pdo_type != APDO_TYPE)
		return max_volt;

	switch (max_volt) {
	case PROG_0V ... (PROG_5V - 1):
		return 0;
	case PROG_5V ... (PROG_9V - 1):
		return 5000;
	case PROG_9V ... (PROG_15V - 1):
		return 9000;
	case PROG_15V ... (PROG_20V - 1):
		return 15000;
	}
	return 20000;
}
EXPORT_SYMBOL(sec_pd_get_apdo_prog_volt);

int sec_pd_get_max_power(unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	return sec_pd_get_apdo_prog_volt(pdo_type, max_volt) * max_curr;
}
EXPORT_SYMBOL(sec_pd_get_max_power);

int sec_pd_get_pdo_power(unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	unsigned int npdo = 0, nmin_volt = 0, nmax_volt = 0, ncurr = 0;
	int nidx = 0, npwr = 0;
	POWER_LIST *pwr;

	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	npdo = (pdo != NULL) ? (*pdo) : 0;
	nmin_volt = (min_volt != NULL) ? (*min_volt) : 0;
	nmax_volt = (max_volt != NULL) ? (*max_volt) : 0;
	ncurr = (max_curr != NULL) ? (*max_curr) : 0;

	if (npdo > g_psink_status->available_pdo_num) {
		pr_err("%s: invalid argument(%d)\n", __func__, npdo);
		return -EINVAL;
	}

	if (npdo != 0) {
		npwr = sec_pd_check_pdo(npdo, nmin_volt, nmax_volt, ncurr);
		nidx = npdo;
	} else {
		int i, anidx = 0, anpwr = 0, tpwr = 0;

		for (i = 1; i <= g_psink_status->available_pdo_num; i++) {
			pwr = &g_psink_status->power_list[i];

			tpwr = sec_pd_check_pdo(i, nmin_volt, nmax_volt, ncurr);
			if (pwr->pdo_type == APDO_TYPE) {
				if (anpwr < tpwr) {
					anidx = i;
					anpwr = tpwr;
				}
			} else {
				if (npwr < tpwr) {
					nidx = i;
					npwr = tpwr;
				}
			}
		}

		if (!npwr) {
			nidx = anidx;
			npwr = anpwr;
		}
	}

	if ((nidx <= 0) || (npwr <= 0)) {
		pr_err("%s: failed to find available pdo(%d)\n", __func__, npwr);
		return npwr;
	}

	/* update values */
	pwr = &g_psink_status->power_list[nidx];

	if (pdo != NULL)
		*pdo = nidx;
	if (min_volt != NULL)
		*min_volt = pwr->min_voltage;
	if (max_volt != NULL)
		*max_volt = pwr->max_voltage;
	if (max_curr != NULL)
		*max_curr = pwr->max_current;

	pr_info("%s: success to find pdo idx = %d, pwr = %d\n", __func__, nidx, npwr);
	return npwr;
}
EXPORT_SYMBOL(sec_pd_get_pdo_power);

int sec_pd_vpdo_auth(int auth, int d2d_type)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_vpdo_auth) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_vpdo_auth(auth, d2d_type);

	return 0;
}
EXPORT_SYMBOL(sec_pd_vpdo_auth);

int sec_pd_change_src(int max_cur)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_change_src) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_change_src(max_cur);

	return 0;
}
EXPORT_SYMBOL(sec_pd_change_src);

int sec_pd_get_apdo_max_power(unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	int i;
	int fpdo_max_power = 0;

	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->has_apdo) {
		pr_info("%s: pd don't have apdo\n", __func__);
		return -1;
	}

	for (i = 1; i <= g_psink_status->available_pdo_num; i++) {
		if (g_psink_status->power_list[i].pdo_type != APDO_TYPE) {
			fpdo_max_power =
				(g_psink_status->power_list[i].max_voltage * g_psink_status->power_list[i].max_current) > fpdo_max_power ?
				(g_psink_status->power_list[i].max_voltage * g_psink_status->power_list[i].max_current) : fpdo_max_power;
		}
	}

	if (*pdo_pos == 0) {
		/* min(max power of all fpdo, max power of selected apdo) */
		for (i = 1; i <= g_psink_status->available_pdo_num; i++) {
			if ((g_psink_status->power_list[i].pdo_type == APDO_TYPE) &&
				g_psink_status->power_list[i].accept &&
				(g_psink_status->power_list[i].max_voltage >= *taMaxVol)) {
				*pdo_pos = i;
				*taMaxVol = g_psink_status->power_list[i].max_voltage;
				*taMaxCur = g_psink_status->power_list[i].max_current;
				*taMaxPwr = min(fpdo_max_power,
					(g_psink_status->power_list[i].max_voltage * g_psink_status->power_list[i].max_current));

				pr_info("%s : *pdo_pos(%d), *taMaxVol(%d), *maxCur(%d), *maxPwr(%d)\n",
					__func__, *pdo_pos, *taMaxVol, *taMaxCur, *taMaxPwr);

				return 0;
			}
		}
	} else {
		/* If we already have pdo object position, we don't need to search max current */
		return -ENOTSUPP;
	}

	pr_info("mv (%d) and ma (%d) out of range of APDO\n", *taMaxVol, *taMaxCur);

	return -EINVAL;
}
EXPORT_SYMBOL(sec_pd_get_apdo_max_power);

void sec_pd_init_data(SEC_PD_SINK_STATUS* psink_status)
{
	g_psink_status = psink_status;
	if (g_psink_status)
		pr_info("%s: done.\n", __func__);
	else
		pr_err("%s: g_psink_status is NULL\n", __func__);
}
EXPORT_SYMBOL(sec_pd_init_data);

int sec_pd_register_chg_info_cb(void *cb)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}
	g_psink_status->fp_sec_pd_ext_cb = cb;

	return 0;
}
EXPORT_SYMBOL(sec_pd_register_chg_info_cb);

void sec_pd_get_vid(unsigned short *vid)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}
	*vid = g_psink_status->vid;
}
EXPORT_SYMBOL(sec_pd_get_vid);

void sec_pd_get_vid_pid(unsigned short *vid, unsigned short *pid, unsigned int *xid)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}
	*vid = g_psink_status->vid;
	*pid = g_psink_status->pid;
	*xid = g_psink_status->xid;
}
EXPORT_SYMBOL(sec_pd_get_vid_pid);

void sec_pd_manual_ccopen_req(int is_on)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}

	if (!g_psink_status->fp_sec_pd_manual_ccopen_req) {
		pr_err("%s: not exist\n", __func__);
		return;
	}

	g_psink_status->fp_sec_pd_manual_ccopen_req(is_on);
}
EXPORT_SYMBOL(sec_pd_manual_ccopen_req);

void sec_pd_manual_jig_ctrl(bool mode)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}

	if (!g_psink_status->fp_sec_pd_manual_jig_ctrl) {
		pr_err("%s: not exist\n", __func__);
		return;
	}

	g_psink_status->fp_sec_pd_manual_jig_ctrl(mode);
}
EXPORT_SYMBOL(sec_pd_manual_jig_ctrl);

static int __init sec_pd_init(void)
{
	pr_info("%s: \n", __func__);
	return 0;
}

module_init(sec_pd_init);
MODULE_DESCRIPTION("Samsung PD control");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
