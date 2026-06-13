/******************************************************************************

			  L I B R M N E T C T L . H

Copyright (c) 2013-2015, 2017-2019, 2021 The Linux Foundation.
All rights reserved.

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

/*!
*  @file    librmnetctl.h
*  @brief   rmnet control API's header file
*/

#ifndef LIBRMNETCTL_H
#define LIBRMNETCTL_H

/* RMNET API succeeded */
#define RMNETCTL_SUCCESS 0
/* RMNET API encountered an error while executing within the library. Check the
* error code in this case */
#define RMNETCTL_LIB_ERR 1
/* RMNET API encountered an error while executing in the kernel. Check the
* error code in this case */
#define RMNETCTL_KERNEL_ERR 2
/* RMNET API encountered an error because of invalid arguments*/
#define RMNETCTL_INVALID_ARG 3

/* Flag to associate a network device*/
#define RMNETCTL_DEVICE_ASSOCIATE 1
/* Flag to unassociate a network device*/
#define RMNETCTL_DEVICE_UNASSOCIATE 0
/* Flag to create a new virtual network device*/
#define RMNETCTL_NEW_VND 1
/* Flag to free a new virtual network device*/
#define RMNETCTL_FREE_VND 0
/* Flag to add a new flow*/
#define RMNETCTL_ADD_FLOW 1
/* Flag to delete an existing flow*/
#define RMNETCTL_DEL_FLOW 0

enum rmnetctl_error_codes_e {
	/* API succeeded. This should always be the first element. */
	RMNETCTL_API_SUCCESS = 0,

	RMNETCTL_API_FIRST_ERR = 1,
	/* API failed because not enough memory to create buffer to send
	 * message */
	RMNETCTL_API_ERR_REQUEST_INVALID = RMNETCTL_API_FIRST_ERR,
	/* API failed because not enough memory to create buffer for the
	 *  response message */
	RMNETCTL_API_ERR_RESPONSE_INVALID = 2,
	/* API failed because could not send the message to kernel */
	RMNETCTL_API_ERR_MESSAGE_SEND = 3,
	/* API failed because could not receive message from the kernel */
	RMNETCTL_API_ERR_MESSAGE_RECEIVE = 4,

	RMNETCTL_INIT_FIRST_ERR = 5,
	/* Invalid process id. So return an error. */
	RMNETCTL_INIT_ERR_PROCESS_ID = RMNETCTL_INIT_FIRST_ERR,
	/* Invalid socket descriptor id. So return an error. */
	RMNETCTL_INIT_ERR_NETLINK_FD = 6,
	/* Could not bind the socket to the Netlink file descriptor */
	RMNETCTL_INIT_ERR_BIND = 7,
	/* Invalid user id. Only root has access to this function. (NA) */
	RMNETCTL_INIT_ERR_INVALID_USER = 8,

	RMNETCTL_API_SECOND_ERR = 9,
	/* API failed because the RmNet handle for the transaction was NULL */
	RMNETCTL_API_ERR_HNDL_INVALID = RMNETCTL_API_SECOND_ERR,
	/* API failed because the request buffer for the transaction was NULL */
	RMNETCTL_API_ERR_REQUEST_NULL = 10,
	/* API failed because the response buffer for the transaction was NULL*/
	RMNETCTL_API_ERR_RESPONSE_NULL = 11,
	/* API failed because the request and response type do not match*/
	RMNETCTL_API_ERR_MESSAGE_TYPE = 12,
	/* API failed because the return type is invalid */
	RMNETCTL_API_ERR_RETURN_TYPE = 13,
	/* API failed because the string was truncated */
	RMNETCTL_API_ERR_STRING_TRUNCATION = 14,

	/* These error are 1-to-1 with rmnet_data config errors in rmnet_data.h
	   for each conversion.
	   please keep the enums synced.
	*/
	RMNETCTL_KERNEL_FIRST_ERR = 15,
	/* No error */
	RMNETCTL_KERNEL_ERROR_NO_ERR = RMNETCTL_KERNEL_FIRST_ERR,
	/* Invalid / unsupported message */
	RMNETCTL_KERNEL_ERR_UNKNOWN_MESSAGE = 16,
	/* Internal problem in the kernel module */
	RMNETCTL_KERNEL_ERR_INTERNAL = 17,
	/* Kernel is temporarily out of memory */
	RMNETCTL_KERNEL_ERR_OUT_OF_MEM = 18,
	/* Device already exists / Still in use */
	RMETNCTL_KERNEL_ERR_DEVICE_IN_USE = 19,
	/* Invalid request / Unsupported scenario */
	RMNETCTL_KERNEL_ERR_INVALID_REQUEST = 20,
	/* Device doesn't exist */
	RMNETCTL_KERNEL_ERR_NO_SUCH_DEVICE = 21,
	/* One or more of the arguments is invalid */
	RMNETCTL_KERNEL_ERR_BAD_ARGS = 22,
	/* Egress device is invalid */
	RMNETCTL_KERNEL_ERR_BAD_EGRESS_DEVICE = 23,
	/* TC handle is full */
	RMNETCTL_KERNEL_ERR_TC_HANDLE_FULL = 24,

	RMNETCTL_API_THIRD_ERR = 25,
	/* Failed to copy data into netlink message */
	RMNETCTL_API_ERR_RTA_FAILURE = RMNETCTL_API_THIRD_ERR,

	/* This should always be the last element */
	RMNETCTL_API_ERR_ENUM_LENGTH
};

#define RMNETCTL_ERR_MSG_SIZE 100

/*!
* @brief Contains a list of error message from API
*/

#define RMNETCTL_LL_MASK_ACK 1
#define RMNETCTL_LL_MASK_RETRY 2

enum rmnetctl_ll_status_e {
	LL_STATUS_ERROR = 0,
	LL_STATUS_SUCCESS = 1,
	LL_STATUS_DEFAULT = 2,
	LL_STATUS_LL = 3,
	LL_STATUS_TEMP_FAIL = 4,
	LL_STATUS_PERM_FAIL = 5,
	LL_STATUS_NO_EFFECT = 0xFD,
	LL_STATUS_TIMEOUT = 0xFE
};

struct rmnetctl_ll_ack
{
	uint8_t bearer_id;
	uint8_t status_code;
	uint8_t current_ch;
};

/*===========================================================================
			 DEFINITIONS AND DECLARATIONS
===========================================================================*/
typedef struct rmnetctl_hndl_s rmnetctl_hndl_t;

/* @brief Public API to initialize the RTM_NETLINK RMNET control driver
 * @details Allocates memory for the RmNet handle. Creates and binds to a
 * netlink socket if successful
 * @param **rmnetctl_hndl_t_val RmNet handle to be initialized
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_init(rmnetctl_hndl_t **hndl, uint16_t *error_code);

/* @brief Public API to clean up the RTM_NETLINK RmNeT control handle
 * @details Close the socket and free the RmNet handle
 * @param *rmnetctl_hndl_t_val RmNet handle to be initialized
 * @return void
 */
int rtrmnet_ctl_deinit(rmnetctl_hndl_t *hndl);

/* @brief Public API to create a new virtual device node
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name Device name new node will be connected to
 * @param vnd_name Name of virtual device to be created
 * @param error_code Status code of this operation returned from the kernel
 * @param index Index node will have
 * @param flagconfig Flag configuration device will have
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_newvnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
		       uint16_t *error_code, uint8_t  index,
		       uint32_t flagconfig);

/* @brief Public API to delete a virtual device node
 * @details Message type is RTM_DELLINK
 * @param hndl RmNet handle for the Netlink message
 * @param vnd_name Name of virtual device to be deleted
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_delvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code);

/* @brief Public API to change flag's of a virtual device node
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name Name of device node is connected to
 * @param vnd_name Name of virtual device to be changed
 * @param error_code Status code of this operation returned from the kernel
 * @param flagconfig New flag config vnd should have
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_changevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code, uint8_t  index,
			  uint32_t flagconfig);

/* @brief Public API to retrieve configuration of a virtual device node
 * @details Message type is RTM_GETLINK
 * @param hndl RmNet handle for the Netlink message
 * @param vndname Name of virtual device to query
 * @param error_code Status code of this operation returned from the kernel
 * @param mux_id Where to store the value of the node's mux id
 * @param flagconfig Where to store the value of the node's data format flags
 * @param agg_count Where to store the value of the node's maximum packet count
 * for uplink aggregation
 * @param agg_size Where to store the value of the node's maximum byte count
 * for uplink aggregation
 * @param agg_time Where to store the value of the node's maximum time limit
 * for uplink aggregation
 * @param agg_time Where to store the value of the node's features
 * for uplink aggregation
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARF if invalid arguments were passed to the API
 */
int rtrmnet_ctl_getvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code, uint16_t *mux_id,
		       uint32_t *flagconfig, uint8_t *agg_count,
		       uint16_t *agg_size, uint32_t *agg_time,
		       uint8_t *features);

/* @brief Public API to bridge a vnd and device
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name device to bridge msg will be sent to
 * @param vnd_name vnd name of device that will be dev_name's master
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_bridgevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code);

/* @brief Public API to configure the uplink aggregation parameters
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param packet_count Maximum number of packets to aggregate
 * @param byte_count Maximum number of bytes to aggregate
 * @param time_limit Maximum time to aggregate
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_set_uplink_aggregation_params(rmnetctl_hndl_t *hndl,
					  char *devname,
					  char *vndname,
					  uint8_t packet_count,
					  uint16_t byte_count,
					  uint32_t time_limit,
					  uint8_t features,
					  uint16_t *error_code);

/* @brief Public API to configure the uplink aggregation parameters
 * used by the RmNet driver for the Low Latency channel
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param packet_count Maximum number of packets to aggregate
 * @param byte_count Maximum number of bytes to aggregate
 * @param time_limit Maximum time to aggregate
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_set_ll_uplink_aggregation_params(rmnetctl_hndl_t *hndl,
					     char *devname,
					     char *vndname,
					     uint8_t packet_count,
					     uint16_t byte_count,
					     uint32_t time_limit,
					     uint8_t features,
					     uint16_t *error_code);

int rtrmnet_activate_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint32_t tcm_handle,
			  uint16_t *error_code);

int rtrmnet_delete_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint16_t *error_code);




int rtrmnet_control_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint16_t sequence,
			  uint32_t grantsize,
			  uint8_t ack,
			  uint16_t *error_code);

int rtrmnet_flow_state_down(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint16_t *error_code);


int rtrmnet_flow_state_up(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint32_t ep_type,
			  uint32_t ifaceid,
			  int flags,
			  uint16_t *error_code);

int rtrmnet_set_qmi_scale(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t scale,
			  uint16_t *error_code);

int rtrmnet_set_wda_freq(rmnetctl_hndl_t *hndl,
			 char *devname,
			 char *vndname,
			 uint32_t freq,
			 uint16_t *error_code);

int rtrmnet_change_bearer_channel(rmnetctl_hndl_t *hndl,
				  char *devname,
				  char *vndname,
				  uint8_t switch_type,
				  uint32_t flags,
				  uint8_t num_bearers,
				  uint8_t *bearers,
				  uint16_t *error_code);

int rtrmnet_get_ll_ack(rmnetctl_hndl_t *hndl,
		       struct rmnetctl_ll_ack *ll_ack,
		       uint16_t *error_code);

const char *rtrmnet_ll_status_to_text(uint8_t status);

#endif /* not defined LIBRMNETCTL_H */

