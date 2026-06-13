/*
 * FST CLI based main
 *
 * Copyright (c) 2015,2019, The Linux Foundation. All rights reserved.
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
#include <signal.h>
#include <stdbool.h>

#include "utils/common.h"
#include "utils/os.h"
#include "utils/eloop.h"
#include "common/defs.h"

#define FST_MGR_COMPONENT "MAINCLI"
#include "fst_manager.h"
#include "fst_ctrl.h"
#include "fst_cfgmgr.h"
#ifdef ANDROID
#include "fst_capconfigstore.h"
#include "fst_hidl.h"
#endif

#define DEFAULT_FST_INIT_RETRY_PERIOD_SEC 1
#define MAX_CTRL_IFACE_SIZE 256

extern Boolean fst_ctrl_create(const char *ctrl_iface,
	unsigned int ping_interval);
extern void fst_ctrl_free(void);

/* globals */
unsigned int fst_debug_level = MSG_INFO;
unsigned int fst_num_of_retries = 20;
unsigned int fst_ping_interval = 1;
Boolean      fst_force_nc = FALSE;
static Boolean fst_main_do_loop = FALSE;
static Boolean fst_continuous_loop = FALSE;
static Boolean terminate_signalled = FALSE;
static Boolean init_done = FALSE;

static void fst_manager_signal_terminate(int sig)
{
	fst_mgr_printf(MSG_INFO, "termination signal arrived (%d)",
			sig);
	terminate_signalled = TRUE;
}

static void fst_manager_eloop_terminate(int sig, void *signal_ctx)
{
	eloop_terminate();
	fst_manager_signal_terminate(sig);
}

void fst_manager_terminate(void)
{
	fst_main_do_loop = FALSE;
	eloop_terminate();
}

static void usage(const char *prog)
{
	printf("Usage: %s [options] [<ctrl_interace_name>]\n", prog);
	printf(", where options are:\n"
	       "\t--version, -V       - show version.\n"
	       "\t--daemon, -B        - run in daemon mode. Exit after "
			"wpa_supplicant/hostapd quits\n"
	       "\t--daemonex, -b      - continuous daemon mode. Wait for a next "
			"wpa_supplicant/hostapd\n"
	       "\t--config, -c <file> - read the FST configuration from the file\n"
	       "\t--retries -r <int>  - number of session setup retries.\n"
	       "\t--ping-int -p <int> - CLI ping interval in sec, 0 to disable\n"
	       "\t--force-nc -n       - force non-compliant mode.\n"
	       "\t--debug, -d         - increase debugging verbosity (-dd - more, "
			"-ddd - even more)\n"
	       "\t--logfile, -f <file>- log output to specified file\n"
#ifdef ANDROID
	       "\t--autogen, -a <mode>- auto-generate fstman.ini for android "
			" (mode: 0=supplicant 1=softap)\n"
#endif
	       "\t--usage, -u         - this message\n"
	       "\t--help, -h          - this message\n");
	exit(2);

}

void try_to_init(void *eloop_ctx, void *ctx)
{
	const char *ctrl_iface = (const char *)ctx;

	if (!fst_ctrl_create(ctrl_iface, fst_ping_interval)) {
		fst_mgr_printf(MSG_ERROR, "cannot create fst_ctrl");
		goto again;
	}

	if (fst_manager_init()) {
		fst_mgr_printf(MSG_ERROR, "cannot init fst manager");
		goto error_init;
	}

	init_done = TRUE;

	return;

error_init:
	fst_ctrl_free();
again:
	eloop_register_timeout(DEFAULT_FST_INIT_RETRY_PERIOD_SEC, 0, try_to_init, NULL, (void*)ctrl_iface);
}

void main_loop(const char *ctrl_iface)
{
	if (eloop_register_signal_terminate(fst_manager_eloop_terminate, NULL)) {
		fst_mgr_printf(MSG_ERROR, "eloop_register_signal_terminate");
		return;
	}

	eloop_run();

	fst_mgr_printf(MSG_INFO, "eloop finished");
	if (!fst_continuous_loop)
		fst_main_do_loop = FALSE;

	if (init_done) {
		fst_manager_deinit();
		fst_ctrl_free();
		init_done = FALSE;
	}
}

#ifdef ANDROID

static const char FSTMAN_IFNAME[] = "wlan0";
static const char FSTMAN_WIGIG_IFNAME[] = "wigig0";
static const char FSTMAN_DATA_IFNAME[] = "bond0";
static const char FSTMAN_SAP_IFNAME[] = "wlan0";
static const char FSTMAN_WIGIG_IF_CHANNEL[] = "2";
static const char FST_STA_INTERFACE_KEY_NAME[] =
	"fst.wifi.sta.interface";
static const char FST_SAP_INTERFACE_KEY_NAME[] =
	"fst.wifi.sap.interface";
static const char FST_DATA_INTERFACE_KEY_NAME[] =
	"fst.data.interface";
static const char FST_WIGIG_INTERFACE_KEY_NAME[] =
	"fst.wigig.interface";
static const char FST_WIGIG_INTERFACE_CHANNEL_KEY_NAME[] =
	"fst.wigig.interface.channel";

static int create_fstman_ini_file(int softap_mode, char *buf, int len)
{
	char fst_iface1[64] = { '\0' };
	char fst_iface2[64] = { '\0' };
	char fst_data_iface[64] = { '\0' };
	char fst_wigig_channel[64] = { '\0' };
	int result;

	if (softap_mode)
		fst_get_config_string(FST_SAP_INTERFACE_KEY_NAME,
				      FSTMAN_SAP_IFNAME,
				      fst_iface1, sizeof(fst_iface1));
	else
		fst_get_config_string(FST_STA_INTERFACE_KEY_NAME,
				      FSTMAN_IFNAME,
				      fst_iface1, sizeof(fst_iface1));
	fst_get_config_string(FST_WIGIG_INTERFACE_KEY_NAME, FSTMAN_WIGIG_IFNAME,
			      fst_iface2, sizeof(fst_iface2));
	fst_get_config_string(FST_DATA_INTERFACE_KEY_NAME, FSTMAN_DATA_IFNAME,
			      fst_data_iface, sizeof(fst_data_iface));
	fst_get_config_string(FST_WIGIG_INTERFACE_CHANNEL_KEY_NAME,
			      FSTMAN_WIGIG_IF_CHANNEL,
			      fst_wigig_channel, sizeof(fst_wigig_channel));

	result = snprintf(buf, len,
		 "[fst_manager]\n"
		 "ctrl_iface=/data/vendor/wifi/hostapd/global\n"
		 "groups=%s\n" /* bond0 */
		 "\n"
		 "[%s]\n" /* bond0 */
		 "interfaces=%s,%s\n" /* wlan0,wigig0 */
		 "mux_type=bonding\n"
		 "mux_ifname=%s\n" /* bond0 */
		 "mux_managed=1\n"
		 "mac_address_by=%s\n" /* wlan0 */
		 "rate_upgrade_master=%s\n" /* wlan0 */
		 "txqueuelen=100\n"
		 "rate_upgrade_acl_file=/data/vendor/wifi/fst_rate_upgrade.accept\n"
		 "\n"
		 "[%s]\n" /* wlan0 */
		 "manual_enslave=1\n"
		 "priority=100\n"
		 "default_llt=3600\n"
		 "\n"
		 "[%s]\n" /* wigig0 */
		 "manual_enslave=1\n"
		 "priority=110\n"
		 "wpa_group=GCMP\n"
		 "wpa_pairwise=GCMP\n"
		 "hw_mode=ad\n"
		 "channel=%s\n",
		 fst_data_iface,
		 fst_data_iface,
		 fst_iface1, fst_iface2,
		 fst_data_iface,
		 fst_iface1,
		 fst_iface1,
		 fst_iface1,
		 fst_iface2,
		 fst_wigig_channel
	 );

	return result;
}

static bool write_fstman_ini_file(const char *fname, const char *contents, int len)
{
	FILE *f;
	int written;

	f = fopen(fname, "w");
	if (!f) {
		fst_mgr_printf(MSG_ERROR,
			"fail to open %s for writing\n", fname);
		return false;
	}
	written = fwrite(contents, 1, len, f);
	fclose(f);
	if (written != len) {
		fst_mgr_printf(MSG_ERROR,
			"fail to write to %s\n", fname);
		return false;
	}

	return true;
}

static int fst_manager_gen_fstman_ini(const char *fname, int softap_mode)
{
	char cfg[2048];
	int len;

	len = create_fstman_ini_file(softap_mode, cfg, sizeof(cfg));
	if (len <= 0)
		return -1;
	if (!write_fstman_ini_file(fname, cfg, len))
		return -1;

	return 0;
}

#endif /* ANDROID */

int main(int argc, char *argv[])
{
	const struct option long_opts[] = {
		{"version",  no_argument, NULL, 'V'},
		{"daemon",   no_argument, NULL, 'B'},
		{"daemonex", no_argument, NULL, 'b'},
		{"config",   required_argument, NULL, 'c'},
		{"retries",  required_argument, NULL, 'r'},
		{"ping-int",  required_argument, NULL, 'p'},
		{"force-nc", no_argument, NULL, 'n'},
		{"debug",    optional_argument, NULL, 'd'},
		{"logfile",  required_argument, NULL, 'f'},
		{"autogen",  required_argument, NULL, 'a'},
		{"usage",    no_argument, NULL, 'u'},
		{"help",     no_argument, NULL, 'h'},
		{}
	};
	int res = -1;
	char short_opts[] = "VBbc:r:nd::f:a:uh";
	const char *ctrl_iface = NULL;
	char *fstman_config_file = NULL;
	int opt, i, mode;
	char buf[MAX_CTRL_IFACE_SIZE];
#ifdef ANDROID
	void *priv;
#endif

	while ((opt = getopt_long_only(argc, argv, short_opts, long_opts, NULL))
	       != -1) {
		switch (opt) {
		case 'V':
			printf("FST Manager, version "
				FST_MANAGER_VERSION "\n");
			exit(0);
			break;
		case 'B':
			fst_main_do_loop = TRUE;
			break;
		case 'b':
			fst_main_do_loop = TRUE;
			fst_continuous_loop = TRUE;
			break;
		case 'c':
			if (fstman_config_file != NULL) {
				fst_mgr_printf(MSG_ERROR,
					"Multiple configurations not allowed\n");
				goto error_cfmgr_params;
			}
			if (optarg != NULL)
				fstman_config_file=os_strdup(optarg);
			if (fstman_config_file == NULL) {
				fst_mgr_printf(MSG_ERROR,
					"Filename memory allocation error\n");
				goto error_cfmgr_params;
			}
			break;
		case 'r':
			if (optarg != NULL)
				fst_num_of_retries = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			if (optarg != NULL)
				fst_ping_interval = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			fst_force_nc = TRUE;
			fst_mgr_printf(MSG_INFO, "Non-compliant FST mode forced\n");
			break;
		case 'd':
			fst_debug_level = MSG_DEBUG;
			if (optarg && optarg[0] == 'd') {
				fst_debug_level = MSG_MSGDUMP;
				if (optarg[1] == 'd')
					fst_debug_level = MSG_EXCESSIVE;
			}
			break;
		case 'f':
			if (wpa_debug_open_file(optarg))  {
				fst_mgr_printf(MSG_ERROR,
					"Cannot open log file: %s", optarg);
				goto error_cfmgr_params;
			}
			break;
#ifdef ANDROID
		case 'a':
			if (fstman_config_file == NULL) {
				fst_mgr_printf(MSG_ERROR,
					"Config file must be specified\n");
				goto error_cfmgr_params;
			}
			mode = optarg ? strtoul(optarg, NULL, 0) : 0;
			res = fst_manager_gen_fstman_ini(
				fstman_config_file, mode);
			if (res < 0) {
				fst_mgr_printf(MSG_ERROR,
					"fstman.ini gen failure, err: %d\n",
					res);
				goto error_cfmgr_params;
			}
			break;
#endif
		case 'u':
		case 'h':
		case '?':
		default:
			usage(argv[0]);
			break;
		}
	}

	if (!fstman_config_file && argc - optind != 1) {
		fst_mgr_printf(MSG_ERROR,
			"either ctrl_interace_name or config has to be specified");
		usage(argv[0]);
		goto error_cfmgr_params;
	}

	if (fstman_config_file)
		i = fst_cfgmgr_init(FST_CONFIG_INI, (void*)fstman_config_file);
	else
		i = fst_cfgmgr_init(FST_CONFIG_CLI, NULL);
	if (i != 0) {
		fst_mgr_printf(MSG_ERROR, "FST Configuration error");
		goto error_cfmgr_init;
	}

	if (argc - optind == 1)
		ctrl_iface = argv[optind];
	else if (!fst_cfgmgr_get_ctrl_iface(buf, sizeof(buf)))
		ctrl_iface = buf;
	else {
		fst_mgr_printf(MSG_ERROR, "cannot get ctrl_iface");
		goto error_ctrl_iface;
	}

	if (eloop_init() != 0)  {
		fst_mgr_printf(MSG_ERROR, "cannot init eloop");
		goto error_eloop_init;
	}

#ifdef ANDROID
	priv = fst_hidl_init();
	if (!priv) {
		fst_mgr_printf(MSG_ERROR, "cannot init HIDL");
		goto error_hidl_init;
	}
#endif

	signal(SIGINT, fst_manager_signal_terminate);
	signal(SIGTERM, fst_manager_signal_terminate);
	while (TRUE) {
		eloop_cancel_timeout(try_to_init, NULL, NULL);
		eloop_register_timeout(0, 0, try_to_init, NULL, (void*)ctrl_iface);
		main_loop(ctrl_iface);
		if (!fst_main_do_loop || terminate_signalled)
			break;
		signal(SIGINT, fst_manager_signal_terminate);
		signal(SIGTERM, fst_manager_signal_terminate);
		os_sleep(DEFAULT_FST_INIT_RETRY_PERIOD_SEC, 0);
	}

	res = 0;
#ifdef ANDROID
	fst_hidl_deinit(priv);
#endif

error_hidl_init:
	eloop_destroy();
error_eloop_init:
error_ctrl_iface:
	fst_cfgmgr_deinit();
error_cfmgr_init:
error_cfmgr_params:
	wpa_debug_close_file();
	os_free(fstman_config_file);
	return res;
}
