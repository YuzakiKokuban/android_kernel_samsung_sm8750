/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __CAM_INTER_VM_COMMS_H__
#define __CAM_INTER_VM_COMMS_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/bug.h>

/**
 * @brief cam_intervm_response_data
 *
 * @param result           : The result from the message handler.
 * @param msg_buffer       : The message ID that this response is for.
 */
struct cam_intervm_response_data {
	int result;
	uint32_t msg_id;
};

/**
 * @brief cam_intervm_response
 *
 * @param send_response     : Whether or not to send a response.
 * @res_msg                 : Response message
 * @msg_size                : Response message size.
 */
struct cam_intervm_response {
	bool send_response;
	void *res_msg;
	size_t msg_size;
};

/**
 * @brief Signature of the callback function to handle incoming messages.
 *        It is advised to create a work to process this message
 *        asynchronously on the client side.
 *
 * @param data        : Incoming message data.
 * @param msg_size    : Size of the incoming message.
 * @res               : Response to the message.
 *
 * @return void.
 */
typedef void (*handle_message_cb) (void *data, size_t msg_size, struct cam_intervm_response *res);
/**
 * cam_inter_vm_comms_ops : Operations allowed for
 *                          inter VM communication.
 */
struct cam_inter_vm_comms_ops {
	/**
	 * @brief Initializes the comms handle with the given
	 *        attributes and initiates the connection.
	 *
	 * @param handle      : The comms handle for this VM.
	 * @param msg_size    : The highest size of the messages exchanged.
	 * @param msg_cb      : The callback function to handle the incoming
	 *                      messages.
	 * @param is_server_vm: Indicated whether this is a primary VM.
	 *
	 * @return Status of operation. Negative in case of error. Zero otherwise.
	 */
	int (*init)(void **handle, size_t msg_size,
		handle_message_cb msg_cb, bool is_server_vm);

	/**
	 * @brief Sends the message to the secondary VM if the
	 *        communication has been established.
	 *
	 * @param handle      : The comms handle for this VM.
	 * @param data        : The data to be sent over.
	 * @param size        : The size of the data to be sent.
	 *
	 * @return Status of operation. Negative in case of error. Zero otherwise.
	 */
	int (*send_message)(void *handle, void *data, size_t size);

	/**
	 * @brief Closes the communication and un-initialize the handle.
	 *
	 * @param handle      : The comms handle for this VM.
	 *
	 * @return Status of operation. Negative in case of error. Zero otherwise.
	 */
	int (*deinit)(void *handle);
};

#endif /*__CAM_INTER_VM_COMMS_H__*/
