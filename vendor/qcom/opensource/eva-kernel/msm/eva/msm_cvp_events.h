/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#if !defined(_MSM_CVP_EVENTS_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _MSM_CVP_EVENTS_H_

#include <linux/types.h>
#include <linux/tracepoint.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM msm_cvp

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE msm_cvp_events

// Since Chrome supports to parse the event “tracing_mark_write” by default
// so we can re-use this to display your own events in Chrome
// enable command as below:
// adb shell "echo 1 > /sys/kernel/tracing/events/msm_cvp/tracing_mark_write/enable"
TRACE_EVENT(tracing_mark_write,
	TP_PROTO(int pid, const char *name, bool trace_begin),
	TP_ARGS(pid, name, trace_begin),
	TP_STRUCT__entry(
		__field(int, pid)
		__string(trace_name, name)
		__field(bool, trace_begin)
	),
	TP_fast_assign(
		__entry->pid = pid;
		__assign_str(trace_name, name);
		__entry->trace_begin = trace_begin;
		),
	TP_printk("%s|%d|%s", __entry->trace_begin ? "B" : "E",
		__entry->pid, __get_str(trace_name))
)
#define CVPKERNEL_ATRACE_END(name) \
		trace_tracing_mark_write(current->tgid, name, 0)
#define CVPKERNEL_ATRACE_BEGIN(name) \
		trace_tracing_mark_write(current->tgid, name, 1)

TRACE_EVENT(tracing_eva_frame_from_sw,
	TP_PROTO(u64 aon_cycles, const char *name,
	u32 session_id, u32 stream_id,
	u32 packet_id, u32 transaction_id, u64 ktid),
	TP_ARGS(aon_cycles, name, session_id, stream_id, packet_id, transaction_id, ktid),
	TP_STRUCT__entry(
		__field(u64, aon_cycles)
		__string(trace_name, name)
		__field(u32, session_id)
		__field(u32, stream_id)
		__field(u32, packet_id)
		__field(u32, transaction_id)
		__field(u64, ktid)
	),
	TP_fast_assign(
		__entry->aon_cycles = aon_cycles;
		__assign_str(trace_name, name);
		__entry->session_id = session_id;
		__entry->stream_id  = stream_id;
		__entry->packet_id  = packet_id;
		__entry->transaction_id = transaction_id;
		__entry->ktid = ktid;
	),
	TP_printk("AON_TIMESTAMP: %llu %s session_id = 0x%08x stream_id = 0x%08x packet_id = 0x%08x transaction_id = 0x%08x ktid = %llu",
		__entry->aon_cycles, __get_str(trace_name),
		__entry->session_id, __entry->stream_id,
		__entry->packet_id, __entry->transaction_id, __entry->ktid)
)

TRACE_EVENT(tracing_eva_frame_from_fw,

	TP_PROTO(char *trace),

	TP_ARGS(trace),

	TP_STRUCT__entry(
		__string(trace_name, trace)
	),

	TP_fast_assign(
		__assign_str(trace_name, trace);
	),

	TP_printk("%s", __get_str(trace_name))
);

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#include <trace/define_trace.h>
