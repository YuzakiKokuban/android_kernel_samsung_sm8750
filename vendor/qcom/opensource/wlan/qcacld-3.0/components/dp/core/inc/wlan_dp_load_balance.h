/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _WLAN_DP_LOAD_BALACE_H_
#define _WLAN_DP_LOAD_BALACE_H_

#include "wlan_init_cfg.h"

struct wlan_dp_psoc_context;

#define ALLOWED_BUFFER_WT 5

/**
 * struct wlan_dp_rx_ring_wtg - per ring weightage
 * @weight: weightage of the ring in percentage
 * @ring_id: ring id
 */
struct wlan_dp_rx_ring_wtg {
	uint32_t weight;
	uint8_t ring_id;
};

/**
 * struct cpu_load - structure holding cpu load in percentage
 * @allowed_wtg: allowed wlan weight on this CPU
 * @total_cpu_avg_load: total cpu average load in percentage
 * @wlan_avg_load: wlan average load on the cpu in percentage
 * @cpu_id: cpu id
 */
struct cpu_load {
	int allowed_wtg;
	uint8_t total_cpu_avg_load;
	uint8_t wlan_avg_load;
	uint8_t cpu_id;
};

/**
 * struct cpu_irq_load - structure holding cpu irq load
 * @irq_time_prev: previous CPU irq time in ns
 * @wlan_irq_time_prev: previous wlan irq time in ns
 * @wlan_ksoftirqd_time_prev: previous wlan ksoftirqd time in ns
 * @last_avg_calc_time: time at which last average computed
 * @cpu_id: cpu id
 * @total_cpu_avg_load: Total average cpu load in percentage
 * @wlan_avg_load: wlan average load on cpu in percentage
 */
struct cpu_irq_load {
	uint64_t irq_time_prev;
	uint64_t wlan_irq_time_prev;
	uint64_t wlan_ksoftirqd_time_prev;
	uint64_t last_avg_calc_time;
	uint32_t cpu_id;
	uint8_t total_cpu_avg_load;
	uint8_t wlan_avg_load;
};

/**
 * struct wlan_dp_lb_data - structure holding data for load balance
 * @cpu_load: cpu irq load for NR_CPUS
 * @def_cpumask: default cpu mask used for load balance
 * @preferred_cpu_mask: preferred cpu mask for the load balance which will get
 * changed if all the cups of current preferred mask goes offline or current
 * preferred mask intersect with the audio used cpumask.
 * @curr_cpu_mask: cpus which are available for load balance currently,
 * @audio_taken_cpumask: cpu mask which is being used by audio
 * @cpu_mask_change_time: time in ns at which cpu mask changed last time
 * subset of preferred_cpu_mask
 * @load_balance_lock: lock to protect load balace from multiple contexts
 * @last_stats_avg_comp_time: time since previous stats average computed in ns
 * @last_load_balanced_time: time since when the previous load balance is done
 *	in nanoseconds
 * @num_wlan_used_rx_rings: number of rx rings used by wlan
 * @in_default_affinity: currently default affinity is set for dp rx interrupts
 * @preferred_mask_change_by_cpuhp: preferred cpumask change due to cpu hotplug
 * @reo_cpu_map: REO to CPU mapping
 */
struct wlan_dp_lb_data {
	struct cpu_irq_load cpu_load[NR_CPUS];
	qdf_cpu_mask def_cpumask;
	qdf_cpu_mask preferred_cpu_mask;
	qdf_cpu_mask curr_cpu_mask;
	qdf_cpu_mask audio_taken_cpumask;
	uint64_t cpu_mask_change_time;
	qdf_spinlock_t load_balance_lock;
	uint64_t last_stats_avg_comp_time;
	uint64_t last_load_balanced_time;
	uint8_t num_wlan_used_rx_rings;
	bool in_default_affinity;
	bool preferred_mask_change_by_cpuhp;
	uint8_t reo_cpu_map[8];
};

#ifdef WLAN_DP_LOAD_BALANCE_SUPPORT
/**
 * wlan_dp_load_balancer_init() - initialize load balancer module
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_init(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dp_load_balancer_deinit() - deinitialize load balancer module
 * @psoc: psoc handle
 *
 * Return: None
 */
void wlan_dp_load_balancer_deinit(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dp_lb_compute_stats_average() - compute stats average
 * for load balance
 * @dp_ctx: dp context
 * @cur_tput_level: current throughput level
 *
 * Return: none
 */
void wlan_dp_lb_compute_stats_average(struct wlan_dp_psoc_context *dp_ctx,
				      enum tput_level cur_tput_level);

/**
 * wlan_dp_lb_set_default_affinity() - set default affinity for dp rx
 * groups
 * @dp_ctx: dp context
 *
 * Return: none
 */
void wlan_dp_lb_set_default_affinity(struct wlan_dp_psoc_context *dp_ctx);
#else
static inline void wlan_dp_load_balancer_init(struct wlan_objmgr_psoc *psoc)
{
}

static inline void wlan_dp_load_balancer_deinit(struct wlan_objmgr_psoc *psoc)
{
}

static inline void
wlan_dp_lb_compute_stats_average(struct wlan_dp_psoc_context *dp_ctx,
				 enum tput_level cur_tput_level)
{
}

static inline void
wlan_dp_lb_set_default_affinity(struct wlan_dp_psoc_context *dp_ctx)
{
}
#endif
#endif /* _WLAN_DP_LOAD_BALACE_H_ */
