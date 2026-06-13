/*
 * FST Manager traffic poller implementation
 *
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#define FST_MGR_COMPONENT "TPOLL"
#include "fst_manager.h"
#include "fst_tpoll.h"
#include "fst_cfgmgr.h"
#include "fst_rateupg.h"
#include "fst_capconfigstore.h"

#define FST_TPOLL_MIN_SAMPLES 5
#define Mbps2Bpms(t) (t * 1000 * 1000 / 1000 / 8)
#define OS_RELTIME2MS(t) (t.sec * 1000 + t.usec / 1000)

static const char FST_TPOLL_LOW_THRESH_KEY_NAME[] = "fst.tpoll.low.thresh.mbps";
#define FST_TPOLL_LOW_TRAFFIC_THRESH "10"
static const char FST_TPOLL_HIGH_THRESH_KEY_NAME[] = "fst.tpoll.high.thresh.mbps";
#define FST_TPOLL_HIGH_TRAFFIC_THRESH "40"

enum fst_tpoll_state {
	FST_TPOLL_STATE_DETECTED_NONE,
	FST_TPOLL_STATE_DETECTED_HIGH,
	FST_TPOLL_STATE_DETECTED_LOW,
};

struct fst_tpoll
{
	const struct fst_group_info *group;
	char mux_ifname[IFNAMSIZ + 1];
	int interval_ms;
	int alpha;
	int low_thresh_Bpms;
	int high_thresh_Bpms;

	enum fst_tpoll_state state;

	struct os_reltime prev_sample_time;
	unsigned long long prev_total_bytes;
	int prev_avg; /* bytes/ms */
	size_t num_samples;
};

static struct fst_tpoll g_fst_tpoll;

static int fst_tpoll_read_bytes(unsigned long long *total_bytes)
{
	char fname[128];
	FILE *f;
	unsigned long long rx_bytes, tx_bytes;
	int res = -1;

	if (snprintf(fname, sizeof(fname), "/sys/class/net/%s/statistics/rx_bytes", g_fst_tpoll.mux_ifname) < 0)
		return -1;

	f = fopen(fname, "r");
	if (!f) {
		fst_mgr_printf(MSG_ERROR, "failed to open %s", fname);
		return -1;
	}

	if (fscanf(f, "%llu", &rx_bytes) != 1) {
		fst_mgr_printf(MSG_ERROR, "failed to read %s", fname);
		goto out;
	}

	fclose(f);

	if (snprintf(fname, sizeof(fname), "/sys/class/net/%s/statistics/tx_bytes", g_fst_tpoll.mux_ifname) < 0)
		return -1;

	f = fopen(fname, "r");
	if (!f) {
		fst_mgr_printf(MSG_ERROR, "failed to open %s", fname);
		return -1;
	}

	if (fscanf(f, "%llu", &tx_bytes) != 1) {
		fst_mgr_printf(MSG_ERROR, "failed to read %s", fname);
		goto out;
	}

	*total_bytes = rx_bytes + tx_bytes;
	res = 0;

out:
	fclose(f);
	return res;
}

static void fst_tpoll_timer(void *eloop_ctx, void *timeout_ctx)
{
	unsigned long long total_bytes;
	int res, tput, new_avg = g_fst_tpoll.prev_avg;
	Boolean detected_high = FALSE, detected_low = FALSE;

	res = fst_tpoll_read_bytes(&total_bytes);
	if (res)
		goto out;

	if (total_bytes < g_fst_tpoll.prev_total_bytes) {
		fst_mgr_printf(MSG_WARNING, "invalid sample, total_bytes (%llu) < prev_total_bytes (%llu)", total_bytes, g_fst_tpoll.prev_total_bytes);
		goto out;
	}

	if (g_fst_tpoll.num_samples > 0) {
		struct os_reltime age;

		os_reltime_age(&g_fst_tpoll.prev_sample_time, &age);
		/* bytes/ms */
		tput = (total_bytes - g_fst_tpoll.prev_total_bytes) / OS_RELTIME2MS(age);
	} else {
		/* in 1st sample, "prev" fields are uninitialized */
		tput = 0;
	}

	/* Exponential Filtering */
	new_avg = (g_fst_tpoll.alpha * tput + (100 - g_fst_tpoll.alpha) * g_fst_tpoll.prev_avg) / 100;


	if (++g_fst_tpoll.num_samples < FST_TPOLL_MIN_SAMPLES) {
		/* not enough samples to make decisions */
		goto out;
	}

	switch (g_fst_tpoll.state) {
	case FST_TPOLL_STATE_DETECTED_NONE:
		if (new_avg > g_fst_tpoll.high_thresh_Bpms)
			detected_high = TRUE;
		else if (new_avg < g_fst_tpoll.low_thresh_Bpms)
			detected_low = TRUE;
		break;
	case FST_TPOLL_STATE_DETECTED_HIGH:
		if (new_avg < g_fst_tpoll.low_thresh_Bpms)
			detected_low = TRUE;
		break;
	case FST_TPOLL_STATE_DETECTED_LOW:
		if (new_avg > g_fst_tpoll.high_thresh_Bpms)
			detected_high = TRUE;
		break;
	}

	if (detected_high) {
		fst_mgr_printf(MSG_INFO, "detected high traffic, bytes %llu => %llu, tput %d, avg %d => %d",
			g_fst_tpoll.prev_total_bytes, total_bytes, tput * 8 / 1000,
			g_fst_tpoll.prev_avg * 8 / 1000, new_avg * 8 / 1000);
		fst_rate_upgrade_high_traffic(g_fst_tpoll.group);
		g_fst_tpoll.state = FST_TPOLL_STATE_DETECTED_HIGH;
	} else if (detected_low) {
		fst_mgr_printf(MSG_INFO, "detected low traffic, bytes %llu => %llu, tput %d, avg %d => %d",
			g_fst_tpoll.prev_total_bytes, total_bytes, tput * 8 / 1000,
			g_fst_tpoll.prev_avg * 8 / 1000, new_avg * 8 / 1000);
		fst_rate_upgrade_low_traffic(g_fst_tpoll.group);
		g_fst_tpoll.state = FST_TPOLL_STATE_DETECTED_LOW;
	}

out:
	os_get_reltime(&g_fst_tpoll.prev_sample_time);
	g_fst_tpoll.prev_total_bytes = total_bytes;
	g_fst_tpoll.prev_avg = new_avg;
	eloop_register_timeout(0, g_fst_tpoll.interval_ms * 1000, fst_tpoll_timer, NULL, NULL);
}

int fst_tpoll_start(const struct fst_group_info *group)
{
	int len;

	if (g_fst_tpoll.group != NULL) {
		fst_mgr_printf(MSG_ERROR, "tpoll already started for group %s", g_fst_tpoll.group->id);
		return -1;
	}

	len = fst_cfgmgr_get_mux_ifname(group->id, g_fst_tpoll.mux_ifname, sizeof(g_fst_tpoll.mux_ifname)-1);
	if (len == 0) {
		fst_mgr_printf(MSG_ERROR, "Cannot get mux ifname");
		return -1;
	}

	fst_mgr_printf(MSG_INFO, "starting");

	g_fst_tpoll.group = group;
	g_fst_tpoll.num_samples = 0;
	g_fst_tpoll.state = FST_TPOLL_STATE_DETECTED_NONE;
	g_fst_tpoll.prev_sample_time.sec = g_fst_tpoll.prev_sample_time.usec = 0;
	g_fst_tpoll.prev_total_bytes = 0;
	g_fst_tpoll.prev_avg = 0;

	eloop_register_timeout(0, 0, fst_tpoll_timer, NULL, NULL);

	return 0;
}

void fst_tpoll_stop()
{
	fst_mgr_printf(MSG_INFO, "stopping");

	g_fst_tpoll.group = NULL;
	eloop_cancel_timeout(fst_tpoll_timer, NULL, NULL);
}

void fst_tpoll_pause()
{
	if (g_fst_tpoll.group == NULL)
		return;

	fst_mgr_printf(MSG_INFO, "pausing");

	eloop_cancel_timeout(fst_tpoll_timer, NULL, NULL);
}

void fst_tpoll_resume()
{
	if (g_fst_tpoll.group == NULL)
		return;

	fst_mgr_printf(MSG_INFO, "resuming");

	g_fst_tpoll.num_samples = 0;
	g_fst_tpoll.state = FST_TPOLL_STATE_DETECTED_NONE;
	g_fst_tpoll.prev_sample_time.sec = g_fst_tpoll.prev_sample_time.usec = 0;
	g_fst_tpoll.prev_total_bytes = 0;
	g_fst_tpoll.prev_avg = 0;

	eloop_register_timeout(0, 0, fst_tpoll_timer, NULL, NULL);
}

int fst_tpoll_init(int interval, int alpha)
{
	char buf[64] = { '\0' };

	os_memset(&g_fst_tpoll, 0, sizeof(g_fst_tpoll));

	g_fst_tpoll.interval_ms = interval;
	if (g_fst_tpoll.interval_ms <= 0)
		return -1;

	g_fst_tpoll.alpha = alpha;
	if (g_fst_tpoll.alpha <= 0 || g_fst_tpoll.alpha > 100)
		return -1;

	fst_get_config_string(FST_TPOLL_LOW_THRESH_KEY_NAME,
		FST_TPOLL_LOW_TRAFFIC_THRESH, buf, sizeof(buf));
	g_fst_tpoll.low_thresh_Bpms = Mbps2Bpms(strtoul(buf, NULL, 0));
	if (g_fst_tpoll.low_thresh_Bpms <= 0)
		return -1;

	fst_get_config_string(FST_TPOLL_HIGH_THRESH_KEY_NAME,
		FST_TPOLL_HIGH_TRAFFIC_THRESH, buf, sizeof(buf));
	g_fst_tpoll.high_thresh_Bpms = Mbps2Bpms(strtoul(buf, NULL, 0));
	if (g_fst_tpoll.high_thresh_Bpms <= g_fst_tpoll.low_thresh_Bpms)
		return -1;

	fst_mgr_printf(MSG_INFO, "interval %d, alpha %d, thresh (Bpms) low %d high %d",
		g_fst_tpoll.interval_ms, g_fst_tpoll.alpha, g_fst_tpoll.low_thresh_Bpms, g_fst_tpoll.high_thresh_Bpms);

	return 0;
}

void fst_tpoll_deinit()
{
	eloop_cancel_timeout(fst_tpoll_timer, NULL, NULL);
}
