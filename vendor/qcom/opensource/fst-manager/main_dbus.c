/*
 * FST Dbus based main
 *
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define FST_MGR_COMPONENT "MDBS"
#include "fst_manager.h"

#include "utils/common.h"
#include "utils/os.h"
#include "common/defs.h"

#include "fst_ctrl.h"

#include <glib.h>
#include <glib-unix.h>

struct wpa_ctrl;
extern Boolean fst_ctrl_create(void);
extern void fst_ctrl_free(void);

/* globals */
unsigned int fst_debug_level = MSG_INFO;
unsigned int fst_num_of_retries = 3;
Boolean      fst_force_nc = FALSE;

static gboolean register_signal_terminate(GSourceFunc handler,
		gpointer user_data)
{
	guint id = g_unix_signal_add(SIGINT, handler, user_data);
	if (id > 0) {
		id = g_unix_signal_add(SIGTERM, handler, user_data);
		if (id > 0)
			return TRUE;
	}
	return FALSE;
}

static gboolean fst_manager_terminate(gpointer user_data)
{
	GMainLoop *loop = user_data;
	fst_mgr_printf(MSG_INFO, "termination signal arrived");
	g_main_loop_quit(loop);
	return FALSE;
}

static void usage(const char *prog)
{
	g_print("Usage: %s [options] <ctrl_interace_name>\n", prog);
	g_print(", where options are:\n"
	       "\t--version, -V    - show version.\n"
	       "\t--retries -r     - number of session setup retries.\n"
	       "\t--debug, -d      - debug output\n"
	       "\t--usage, -u      - this message\n"
	       "\t--help, -h       - this message\n");
	exit(2);

}

int main(int argc, char *argv[])
{
	GMainLoop *loop;
	const struct option long_opts[] = {
		{"version", 0, NULL, 'V'},
		{"retries", 1, NULL, 'r'},
		{"force-nc", 0, NULL, 'n'},
		{"debug", 2, NULL, 'd'},
		{"usage", 0, NULL, 'u'},
		{"help", 0, NULL, 'h'},
		{}
	};
	char short_opts[] = "vr:nd::uh";
	int opt;

	while ((opt = getopt_long_only(argc, argv, short_opts, long_opts, NULL))
	       != -1) {
		switch (opt) {
		case 'V':
			printf("FST Manager, version "
				FST_MANAGER_VERSION "\n");
			exit(0);
			break;
		case 'r':
			fst_num_of_retries = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			fst_force_nc = TRUE;
			fst_mgr_printf(MSG_INFO,
				"Non-compliant FST mode forced\n");
			break;
		case 'd':
			fst_debug_level = MSG_EXCESSIVE;
			if (optarg)
				fst_debug_level = strtoul(optarg, NULL, 0);
			break;
		case 'u':
		case 'h':
		case '?':
		default:
			usage(argv[0]);
			break;
		}
	}

	loop = g_main_loop_new(NULL, FALSE);
	if (!loop) {
		fst_mgr_printf(MSG_ERROR, "cannot create main loop");
		goto error_g_main_loop_new;
	}

	if (!fst_ctrl_create()) {
		fst_mgr_printf(MSG_ERROR, "cannot create fst_ctrl");
		goto error_fst_ctrl_create;
	}

	if (!register_signal_terminate(fst_manager_terminate, loop)) {
		fst_mgr_printf(MSG_ERROR, "cannot register termination signal handler");
		goto error_register_signal_terminate;
	}

	g_main_loop_run(loop);

	fst_mgr_printf(MSG_INFO, "main loop finished");

error_register_signal_terminate:
	fst_ctrl_free();
error_fst_ctrl_create:
	g_main_loop_unref(loop);
error_g_main_loop_new:
	return 0;
}
