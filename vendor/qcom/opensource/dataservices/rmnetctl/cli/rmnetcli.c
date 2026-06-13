/******************************************************************************

			R M N E T C L I . C

Copyright (c) 2013-2015, 2017-2021 The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above
	  copyright notice, this list of conditions and the following
	  disclaimer in the documentation and/or other materials provided
	  with the distribution.
	* Neither the name of The Linux Foundation nor the names of its
	  contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

/******************************************************************************

  @file    rmnetcli.c
  @brief   command line interface to expose rmnet control API's

  DESCRIPTION
  File containing implementation of the command line interface to expose the
  rmnet control configuration .

******************************************************************************/

/*===========================================================================
				INCLUDE FILES
===========================================================================*/

#include <sys/socket.h>
#include <stdint.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "rmnetcli.h"
#include "librmnetctl.h"

#define RMNET_MAX_STR_LEN  16

#define _RMNETCLI_CHECKNULL(X)		do { if (!X) {                         \
print_rmnet_api_status(RMNETCTL_INVALID_ARG, RMNETCTL_CFG_FAILURE_NO_COMMAND); \
				rtrmnet_ctl_deinit(handle);                      \
				return RMNETCTL_INVALID_ARG;                   \
		} } while (0);
#define _STRTOUI32(X)           (uint32_t)strtoul(X, NULL, 0)
#define _STRTOUI16(X)           (uint16_t)strtoul(X, NULL, 0)
#define _STRTOUI8(X)           (uint8_t)strtoul(X, NULL, 0)
#define _STRTOI32(X)           (int32_t)strtol(X, NULL, 0)

#define _5TABS 		"\n\t\t\t\t\t"
#define _2TABS 		"\n\t\t"

/*!
* @brief Contains a list of error message from CLI
*/
char rmnetcfg_error_code_text
[RMNETCFG_TOTAL_ERR_MSGS][RMNETCTL_ERR_MSG_SIZE] = {
	"Help option Specified",
	"ERROR: No\\Invalid command was specified\n",
	"ERROR: Could not allocate buffer for Egress device\n"
};

/*!
* @brief Method to display the syntax for the commands
* @details Displays the syntax and usage for the commands
* @param void
* @return void
*/
static void rmnet_api_usage(void)
{
	printf("RmNet API Usage:\n\n");
	printf("rmnetcli help                            Displays this help\n");
	printf("\n");
	printf("**************************\n");
	printf("RmNet RTM_NETLINK API Usage:\n\n");
	printf("rmnetcli -n newlink  <real dev>            Add a vnd w/ newlink");
	printf(_2TABS" <vnd>                   string - vnd device_name");
	printf(_2TABS" <vnd id>                int - new vnd id");
	printf(_2TABS" [flags]                 int - starting flag config\n\n");
	printf("rmnetcli -n changelink  <real dev>         Change a vnd's flags");
	printf(_2TABS" <vnd>                   string - vnd device_name");
	printf(_2TABS" <vnd id>                int - new vnd id");
	printf(_2TABS" <flags>                 int - new flag config\n\n");
	printf("rmnetcli -n getlink <real dev>           Get device config\n\n");
	printf("rmnetcli -n dellink <real dev>           Delete a vnd");
	printf(_2TABS"                         by inputting dev name\n\n");
	printf("rmnetcli -n bridgelink  <real dev>       Bridge a vnd and a dev");
	printf(_2TABS" <vnd id>                by specifying dev id and vnd id\n\n");
	printf("rmnetcli -n uplinkparam <real dev>   set uplink aggregation parameters");
	printf(_2TABS" <vnd id>                string - vnd device_name");
	printf(_2TABS" <packet count>          int - maximum packet count");
	printf(_2TABS" <byte count>            int - maximum byte count");
	printf(_2TABS" <time limit>            int - maximum time limit");
	printf(_2TABS" <features>              int - aggregation features\n\n");
	printf("rmnetcli -n lluplinkparam <dev_name>   set LL uplink aggregation parameters");
	printf(_2TABS" <vnd id>                string - vnd device_name");
	printf(_2TABS" <packet count>          int - maximum packet count");
	printf(_2TABS" <byte count>            int - maximum byte count");
	printf(_2TABS" <time limit>            int - maximum time limit");
	printf(_2TABS" <features>              int - aggregation features\n\n");
	printf("rmnetcli -n flowactivate <real dev>  activate a flow\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>             int - bearer id\n\n");
	printf(_2TABS" <flow id>               int - flow id\n\n");
	printf(_2TABS" <ip type>               int - ip type\n\n");
	printf(_2TABS" <handle>                int - flow handle\n\n");
	printf("rmnetcli -n flowdel      <real dev> delete a flow\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>              int - bearer id\n\n");
	printf(_2TABS" <flow id>               int - flow id\n\n");
	printf(_2TABS" <ip type>               int - ip type\n\n");
	printf("rmnetcli -n flowcontrol  <real dev>");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>             int - bearer id\n\n");
	printf(_2TABS" <seq>                   int - sequence\n\n");
	printf(_2TABS" <grant size>            int - grant size\n\n");
	printf(_2TABS" <ack>                   int - ack\n\n");
	printf("rmnetcli -n systemup      <real dev>\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <instance>              int - bearer id\n\n");
	printf(_2TABS" <eptype>                int - ep type\n\n");
	printf(_2TABS" <iface_id>              int - iface id\n\n");
	printf(_2TABS" <flags>                 int - flags\n\n");
	printf("rmnetcli -n systemdown    <real dev> <vnd name> <instance>\n\n ");
	printf("rmnetcli -n qmiscale      <real dev> set ACK scale factor\n\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <scale>                 int - scale factor\n\n");
	printf("rmnetcli -n wdafreq       <real dev> set powersave poll freq\n\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <freq>                  int - frequency\n\n");
	printf("rmnetcli -n channelswitch  <real dev> change underlying transport channel for bearers");
	printf(_2TABS" <vnd_name>              string - vnd device name");
	printf(_2TABS" <switch type>           =0: from LL, !=0: to LL");
	printf(_2TABS" <flags>                 masks. 1: wait for completion");
	printf(_2TABS" <bearer id list>        int - list of bearer ids to switch\n");
}

static void print_rmnetctl_lib_errors(uint16_t error_number)
{
	if ((error_number > RMNETCTL_API_SUCCESS) &&
		(error_number < RMNETCTL_API_ERR_ENUM_LENGTH)) {
		printf("%s", rmnetctl_error_code_text[error_number]);
	}
	if ((error_number >= RMNETCFG_ERR_NUM_START) &&
	(error_number < RMNETCFG_ERR_NUM_START + RMNETCFG_TOTAL_ERR_MSGS)) {
		printf("%s", rmnetcfg_error_code_text
			[error_number - RMNETCFG_ERR_NUM_START]);
		if ((error_number == RMNETCTL_CFG_SUCCESS_HELP_COMMAND) ||
			(error_number == RMNETCTL_CFG_FAILURE_NO_COMMAND))
			rmnet_api_usage();
	}
}

/*!
* @brief Method to check the error numbers generated from API calls
* @details Displays the error messages based on each error code
* @param error_number Error number returned from the API and the CLI
* @return void
*/
static void print_rmnet_api_status(int return_code, uint16_t error_number)
{
	if (return_code == RMNETCTL_SUCCESS)
		printf("SUCCESS\n");
	else if (return_code == RMNETCTL_LIB_ERR) {
		printf("LIBRARY ");
		print_rmnetctl_lib_errors(error_number);
	} else if (return_code == RMNETCTL_KERNEL_ERR) {
		if (error_number < RMNETCTL_API_ERR_ENUM_LENGTH)
			printf("KERNEL ERROR: System or rmnet error %d\n",
			       error_number);
	}
	else if (return_code == RMNETCTL_INVALID_ARG)
		printf("INVALID_ARG\n");
}

/*!
* @brief Wait for rmnet LL switch status
* @details Waits and displays LL switch status
* @param num_bearers Number of bearers to wait for
* @return RMNETCTL_SUCCESS if successful.
*/
static int rmnet_ll_wait_status(rmnetctl_hndl_t *hndl,
				uint8_t num_bearers,
				uint16_t *error_number)
{
	struct rmnetctl_ll_ack ll_ack;
	int return_code = RMNETCTL_SUCCESS;

	printf("LL switch initiated, waiting for completion...\n");
	printf("Bearer/Status/Channel:\n");
	while (num_bearers--) {
		return_code = rtrmnet_get_ll_ack(hndl, &ll_ack, error_number);
		if (return_code != RMNETCTL_SUCCESS)
			return return_code;

		printf("%u | %u (%s) | %u (%s)\n",
		       ll_ack.bearer_id,
		       ll_ack.status_code,
		       rtrmnet_ll_status_to_text(
				ll_ack.status_code),
		       ll_ack.current_ch,
		       ll_ack.current_ch ? "LL" : "Default");
	}

	return return_code;
}

/*!
* @brief Method to make the API calls
* @details Checks for each type of parameter and calls the appropriate
* function based on the number of parameters and parameter type
* @param argc Number of arguments which vary based on the commands
* @param argv Value of the arguments which vary based on the commands
* @return RMNETCTL_SUCCESS if successful. Relevant data might be printed
* based on the message type
* @return RMNETCTL_LIB_ERR if there was a library error. Error code will be
* printed
* @return RMNETCTL_KERNEL_ERR if there was a error in the kernel. Error code will be
* printed
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/

static int rmnet_api_call(int argc, char *argv[])
{
	struct rmnetctl_hndl_s *handle = NULL;
	uint16_t error_number = RMNETCTL_CFG_FAILURE_NO_COMMAND;
	int return_code = RMNETCTL_LIB_ERR;

	if ((!argc) || (!*argv)) {
		print_rmnet_api_status(RMNETCTL_LIB_ERR,
		RMNETCTL_CFG_FAILURE_NO_COMMAND);
		return RMNETCTL_LIB_ERR;
	}
	if (!strcmp(*argv, "help")) {
		print_rmnet_api_status(RMNETCTL_LIB_ERR,
		RMNETCTL_CFG_SUCCESS_HELP_COMMAND);
		return RMNETCTL_LIB_ERR;
	}

	if (!strcmp(*argv, "-n")) {
		return_code = rtrmnet_ctl_init(&handle, &error_number);
		if (return_code != RMNETCTL_SUCCESS) {
			print_rmnet_api_status(return_code, error_number);
			return RMNETCTL_LIB_ERR;
		}
		error_number = RMNETCTL_CFG_FAILURE_NO_COMMAND;
		return_code = RMNETCTL_LIB_ERR;
		argv++;
		argc--;
		if ((!argc) || (!*argv)) {
			print_rmnet_api_status(RMNETCTL_LIB_ERR,
			RMNETCTL_CFG_FAILURE_NO_COMMAND);
			return RMNETCTL_LIB_ERR;
		}
		if (!strcmp(*argv, "newlink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			uint32_t flags = 0;
			/* If optional flag was used pass it on*/
			if (argv[4])
				flags = _STRTOI32(argv[4]);

			return_code = rtrmnet_ctl_newvnd(handle, argv[1],
							 argv[2],
							 &error_number,
							 _STRTOI32(argv[3]),
							 flags);
		} else if (!strcmp(*argv, "changelink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);

			return_code = rtrmnet_ctl_changevnd(handle, argv[1],
							    argv[2],
							    &error_number,
							    _STRTOI32(argv[3]),
							    _STRTOI32(argv[4]));
		} else if (!strcmp(*argv, "getlink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			uint32_t flags = 0;
			uint16_t mux_id = 0;
			uint8_t agg_count = 0;
			uint16_t agg_size = 0;
			uint32_t agg_time = 0;
			uint8_t features = 0;

			return_code = rtrmnet_ctl_getvnd(handle, argv[1],
							 &error_number,
							 &mux_id, &flags,
							 &agg_count, &agg_size,
							 &agg_time, &features);
			if (return_code == RMNETCTL_API_SUCCESS) {
				printf("Configuration for device %s:\n", argv[1]);
				printf("\tMux id: %d\n", mux_id);
				printf("\tData format: 0x%04x\n", flags);
				printf("\tUplink Aggregation parameters:\n");
				printf("\t\tPacket limit: %d\n", agg_count);
				printf("\t\tByte limit: %d\n", agg_size);
				printf("\t\tTime limit (ns): %d\n", agg_time);
				printf("\t\tFeatures : 0x%02x\n", features);
			}
		} else if (!strcmp(*argv, "dellink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
				return_code = rtrmnet_ctl_delvnd(handle, argv[1],
								 &error_number);
		} else if (!strcmp(*argv, "bridge")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			return_code = rtrmnet_ctl_bridgevnd(handle, argv[1],
							    argv[2],
							    &error_number);
		} else if (!strcmp(*argv, "uplinkparam")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);

			return_code = rtrmnet_set_uplink_aggregation_params(
				handle, argv[1], argv[2], _STRTOUI8(argv[3]),
				_STRTOUI16(argv[4]), _STRTOUI32(argv[5]),
				_STRTOUI8(argv[6]), &error_number);
		} else if (!strcmp(*argv, "lluplinkparam")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);

			return_code = rtrmnet_set_ll_uplink_aggregation_params(
				handle, argv[1], argv[2], _STRTOUI8(argv[3]),
				_STRTOUI16(argv[4]), _STRTOUI32(argv[5]),
				_STRTOUI8(argv[6]), &error_number);
		}
		else if (!strcmp(*argv, "flowactivate")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);

			return_code = rtrmnet_activate_flow(handle, argv[1], argv[2],
							    _STRTOUI8(argv[3]),
							    _STRTOI32(argv[4]),
							    _STRTOUI32(argv[5]),
							    _STRTOUI32(argv[6]),
							    &error_number);

		} else if (!strcmp(*argv, "flowdel")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			return_code = rtrmnet_delete_flow(handle, argv[1], argv[2],
							_STRTOUI8(argv[3]),
							_STRTOUI32(argv[4]),
							_STRTOI32(argv[5]),
							&error_number);
		} else if (!strcmp(*argv, "flowcontrol")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);


			return_code = rtrmnet_control_flow(handle, argv[1], argv[2],
							    _STRTOUI8(argv[3]),
							    _STRTOUI32(argv[4]),
							    _STRTOUI16(argv[5]),
							    _STRTOUI8(argv[6]),
							    &error_number);
		} else if (!strcmp(*argv, "systemup")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);


			return_code = rtrmnet_flow_state_up(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    _STRTOUI32(argv[4]),
							    _STRTOUI32(argv[5]),
							    _STRTOUI32(argv[6]),
							    &error_number);
		} else if (!strcmp(*argv, "systemdown")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_flow_state_down(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    &error_number);
		} else if (!strcmp(*argv, "qmiscale")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_set_qmi_scale(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    &error_number);
		} else if (!strcmp(*argv, "wdafreq")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_set_wda_freq(handle, argv[1], argv[2],
							   _STRTOUI32(argv[3]),
							   &error_number);
		} else if (!strcmp(*argv, "channelswitch")) {
			uint32_t ll_flags;
			uint8_t *bearers;
			int num_bearers = argc - 5;
			int i;

			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			/* Force at least one bearer to be provided */
			_RMNETCLI_CHECKNULL(argv[5]);

			bearers = calloc(sizeof(uint8_t), num_bearers);
			if (!bearers) {
				print_rmnet_api_status(RMNETCTL_INVALID_ARG,
					RMNETCTL_CFG_FAILURE_NO_COMMAND);
				rtrmnet_ctl_deinit(handle);
				return RMNETCTL_INVALID_ARG;
			}

			for (i = 0; i < num_bearers; i++)
				bearers[i] = _STRTOUI8(argv[5 + i]);

			ll_flags =  _STRTOUI32(argv[4]);
			return_code = rtrmnet_change_bearer_channel(handle,
								    argv[1],
								    argv[2],
								    _STRTOUI8(argv[3]),
								    ll_flags,
								    num_bearers,
								    bearers,
								    &error_number);
			free(bearers);

			if (return_code == RMNETCTL_SUCCESS &&
			    (ll_flags & RMNETCTL_LL_MASK_ACK))
				return_code = rmnet_ll_wait_status(
					handle, num_bearers, &error_number);
		}


		goto end;
	}

end:
	print_rmnet_api_status(return_code, error_number);
	rtrmnet_ctl_deinit(handle);

	return return_code;
}

/*!
* @brief Method which serves as en entry point to the rmnetcli function
* @details Entry point for the RmNet Netlink API. This is the command line
* interface for the RmNet API
* @param argc Number of arguments which vary based on the commands
* @param argv Value of the arguments which vary based on the commands
* @return RMNETCTL_SUCCESS if successful. Relevant data might be printed
* based on the message type
* @return RMNETCTL_LIB_ERR if there was a library error. Error code will be
* printed
* @return RMNETCTL_KERNEL_ERR if there was a error in the kernel. Error code will be
* printed
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int main(int argc, char *argv[])
{
	argc--;
	argv++;
	return rmnet_api_call(argc, argv);
}
