/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM samsung

#if !defined(_TRACE_SAMSUNG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SAMSUNG_H

#include <linux/tracepoint.h>

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(int pid, const char *name, unsigned int type, int value),
	TP_ARGS(pid, name, type, value),
	TP_STRUCT__entry(
			__field(int, pid)
			__string(trace_name, name)
			__field(unsigned int, trace_type)
			__field(int, value)
	),
	TP_fast_assign(
			__entry->pid = pid;
			__assign_str(trace_name, name);
			__entry->trace_type = type;
			__entry->value = value;
	),
	TP_printk("%c|%d|%s|%d", __entry->trace_type,
			__entry->pid, __get_str(trace_name), __entry->value)
)
#endif /* _TRACE_SAMSUNG_H */

#define TRACING_MARK_BUF_SIZE 256

#define tracing_mark_begin(fmt, args...)			\
do {								\
	char buf[TRACING_MARK_BUF_SIZE];			\
	if (!trace_tracing_mark_write_enabled())		\
		break;						\
	snprintf(buf, TRACING_MARK_BUF_SIZE, fmt, ##args);	\
	trace_tracing_mark_write(current->tgid, buf, 'B', 0);   \
} while (0)
#define tracing_mark_end()					\
	trace_tracing_mark_write(current->tgid, "", 'E', 0)

/* This part must be outside protection */
#include <trace/define_trace.h>
