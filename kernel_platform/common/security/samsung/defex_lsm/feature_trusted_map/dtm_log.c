// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020-2022 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>

#include "include/dtm.h"
#include "include/dtm_log.h"
#include "include/dtm_utils.h"

#define DTM_MAX_LOG_SIZE	(1024)
#define DTM_MAX_DETAIL_SIZE	(1024)

#ifdef DEFEX_DSMS_ENABLE
#include <linux/dsms.h>
#else
#define DSMS_SUCCESS (0)
#define dsms_send_message(feature_code, message, value) (DSMS_SUCCESS)
#endif

static inline bool should_send_dsms_event(void)
{
#ifdef DEFEX_DSMS_ENABLE
	return IS_ENABLED(CONFIG_SECURITY_DSMS);
#else
	return false;
#endif
}

/* Append as much as possible of *src to *dst, given *dst_size (update both
 * *dst and *dst_size)
 * Escape with '\' any occurrences of sep and '\'; if sep_first, prefix sep.
 * Returns 1 if *str was completely copied, 0 otherwise
 */
static int append(char **dst, size_t *dst_size,
		const char *src, char sep, char sep_first)
{
// Helper to append a single byte, assuming there's space available
#define app_k(k) do { *(*dst)++ = (k); (*dst_size)--; } while (0)
	if (!src)
		return 0;
	if (sep_first) {
		if (*dst_size < 2)
			return 0;
		app_k(sep);
	}
	// TODO support multibyte chars by using correct size instead of 1 and 2
	while (*dst_size > 1 && *src) {
		// escape any separator or escape char itself
		if (*src == '\\' || *src == sep) {
			if (*dst_size > 2)
				app_k('\\');
			else
				break;
		}
		app_k(*src++);
	}
	**dst = 0;
	return *src == 0;
}

__visible_for_testing void dtm_prepare_message(char *message, size_t size,
					       const char *where, const char *sep,
					       struct dtm_context *context)
{
	int total_argc, max_argc, arg, prologue_size;

	/* load all arguments to update attributes and fill arg values */
	total_argc = context->callee_argc;
	max_argc = min_t(int, total_argc, ARRAY_SIZE(context->callee_argv));
	if (context->callee_copied_argc != max_argc)
		for (arg = 0; arg < max_argc; arg++)
			if (!context->callee_argv[arg])
				dtm_get_callee_arg(context, arg);

	snprintf(message, size, "%s%s%d:%d:%ld:%ld:%d", where, sep,
		 context->callee_copied_argc, total_argc,
		 context->callee_copied_args_len,
		 context->callee_total_args_len,
		 CHECK_ROOT_CREDS(context->defex_context->cred));
	/* Note that special treatment is demanded for separators (":"),
	 * quoting (for whitespace), escape sequences and special chars.
	 * Here we only do the essential, escaping separators. Further
	 * treatment enlarges DSMS messages and can/must be dealt with by
	 * log-to-policy tools.
	 */
	prologue_size = strlen(message);
	size -= prologue_size;
	if (size > 0) {
		message += prologue_size;
		if (append(&message, &size,
				dtm_get_caller_path(context), ':', 1) &&
			append(&message, &size,
				dtm_get_callee_path(context), ':', 1) &&
			append(&message, &size,
				dtm_get_stdin_mode(context), ':', 1))
			for (arg = 0; arg < max_argc; ++arg)
				append(&message, &size,
						context->callee_argv[arg]
						? context->callee_argv[arg]
						: "(null)", ':', 1);
	}
}

#ifdef DEFEX_DEBUG_ENABLE
void dtm_debug_call(const char *where, struct dtm_context *context)
{
	char message[DTM_MAX_LOG_SIZE];

	dtm_prepare_message(message, sizeof(message), where, ": ", context);
	DTM_LOG_DEBUG("%s", message);
}
#endif

noinline void dtm_report_violation(const char *feature_code,
				   struct dtm_context *context)
{
	char message[DTM_MAX_DETAIL_SIZE + 1];
	int ret;

	dtm_prepare_message(message, sizeof(message), "", "", context);
	DTM_DEBUG(VIOLATIONS, "[%s]%s", feature_code, message);
	if (should_send_dsms_event()) {
		ret = dsms_send_message(feature_code, message, 0);
		if (unlikely(ret != DSMS_SUCCESS))
			DTM_LOG_ERROR("Error %d while sending DSMS report", ret);
	}
}
