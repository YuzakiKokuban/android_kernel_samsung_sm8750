/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM ngksm

#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_NGKSM_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_NGKSM_H

#include <trace/hooks/vendor_hooks.h>

/* When recovery mode is enabled or disabled by sysfs */
DECLARE_HOOK(android_vh_ngksm,
	TP_PROTO(int *ret, const char *feature_code, const char *detail, int64_t value),
	TP_ARGS(ret, feature_code, detail, value));

#endif /* _TRACE_HOOK_NGKSM_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
