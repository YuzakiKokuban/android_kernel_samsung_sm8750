// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_qrtr_comms.h"
#include "cam_mem_mgr_api.h"

static int cam_qrtr_handle_ctrl_message_locked(struct cam_inter_vm_comms_handle *handle,
	struct qrtr_ctrl_pkt *pkt)
{
	struct cam_qrtr_comms_data *qrtr_data;

	qrtr_data = (struct cam_qrtr_comms_data *)handle->comms_protocol_data;

	/*
	 * For the current requirement for TVM secure camera, no other control
	 * messages need to be processed. When we support graceful TVM shutdown
	 * or PVM camera server shutdown, we need to handle QRTR_TYPE_BYE,
	 * QRTR_TYPE_DEL_SERVER, QRTR_TYPE_DEL_CLIENT.
	 */
	if (!handle->is_server_vm) {
		/*
		 * The only control message expected on the client side is
		 * the'NEW_SERVER' message, before the communication has been
		 * established.
		 */
		if (le32_to_cpu(pkt->cmd) == QRTR_TYPE_NEW_SERVER &&
			!handle->is_comms_established) {
			qrtr_data->remote_sock_addr.sq_node = pkt->server.node;
			qrtr_data->remote_sock_addr.sq_port = pkt->server.port;
			qrtr_data->remote_sock_addr.sq_family = AF_QIPCRTR;
			handle->is_comms_established = true;
			CAM_DBG(CAM_VMRM, "Connection is established");
		}
	} else {
		CAM_DBG(CAM_VMRM, "Unexpected control message on the server side");
		return -EINVAL;
	}

	return 0;
}

static int cam_qrtr_send_message_locked(struct cam_inter_vm_comms_handle *handle,
	void *data, size_t size)
{
	struct kvec iv;
	struct msghdr msg;
	int ret;
	struct cam_qrtr_comms_data *qrtr_data;

	if (size <= 0) {
		CAM_ERR(CAM_VMRM, "Invalid message length");
		return -EINVAL;
	}

	qrtr_data = (struct cam_qrtr_comms_data *)handle->comms_protocol_data;
	msg.msg_name = &qrtr_data->remote_sock_addr;
	msg.msg_namelen = sizeof(qrtr_data->remote_sock_addr);
	iv.iov_base = data;
	iv.iov_len = size;

	ret = kernel_sendmsg(qrtr_data->sock, &msg, &iv, 1, size);
	if (ret < 0) {
		CAM_ERR(CAM_VMRM, "Failed to send message: %d\n", ret);
		mutex_unlock(&handle->comms_lock);
		return ret;
	}

	return 0;
}

static int cam_qrtr_send_response_locked(struct cam_inter_vm_comms_handle *handle,
	void *data, size_t size)
{
	return cam_qrtr_send_message_locked(handle, data, size);
}

static void cam_qrtr_handle_incoming_message(struct work_struct *work)
{
	struct cam_inter_vm_comms_handle *handle;
	ssize_t msg_len;
	struct sockaddr_qrtr sq;
	struct msghdr msg = { .msg_name = &sq, .msg_namelen = sizeof(sq) };
	struct kvec iv;
	struct qrtr_ctrl_pkt *pkt;
	struct cam_qrtr_comms_data *qrtr_data;
	struct cam_intervm_response response;

	handle = container_of(work, struct cam_inter_vm_comms_handle, msg_recv_work);
	if (unlikely(!handle)) {
		CAM_ERR(CAM_VMRM, "Invalid work data");
		return;
	}

	mutex_lock(&handle->comms_lock);
	if (handle->is_comms_terminated) {
		CAM_ERR(CAM_VMRM, "Connection nas been terminated");
		mutex_unlock(&handle->comms_lock);
		return;
	}
	/* Read the messages until the queue is exhausted */
	for (;;) {
		qrtr_data = (struct cam_qrtr_comms_data *)handle->comms_protocol_data;
		iv.iov_base = handle->msg_buffer;
		iv.iov_len = handle->msg_size;

		msg_len = kernel_recvmsg(qrtr_data->sock, &msg, &iv, 1,
			iv.iov_len, MSG_DONTWAIT);
		if (msg_len == -EAGAIN)
			break;
		if (msg_len <= 0) {
			CAM_ERR(CAM_VMRM, "Invalid message received");
			mutex_unlock(&handle->comms_lock);
			return;
		}

		CAM_DBG(CAM_VMRM, "New message received. is_server_vm = %d", handle->is_server_vm);

		if (sq.sq_node == qrtr_data->local_sock_addr.sq_node &&
			sq.sq_port == QRTR_PORT_CTRL) {
			if (msg_len == sizeof(struct qrtr_ctrl_pkt)) {
				pkt = handle->msg_buffer;
				cam_qrtr_handle_ctrl_message_locked(handle, pkt);
			}
		} else {
			/*
			 * Upon the first data message from the client, save the
			 * client address for future communication and set the
			 * status as communication established.
			 */
			CAM_DBG(CAM_VMRM, "New data message received. is_server_vm = %d",
				handle->is_server_vm);
			if (handle->is_server_vm) {
				if (!handle->is_comms_established) {
					qrtr_data->remote_sock_addr.sq_node = sq.sq_node;
					qrtr_data->remote_sock_addr.sq_port = sq.sq_port;
					qrtr_data->remote_sock_addr.sq_family = AF_QIPCRTR;
					handle->is_comms_established = true;
					CAM_DBG(CAM_VMRM, "Connection is established");
				}
			}

			handle->message_cb((void *)handle->msg_buffer, msg_len, &response);
			if (response.send_response) {
				if (cam_qrtr_send_response_locked(handle, response.res_msg,
					response.msg_size))
					CAM_ERR(CAM_VMRM, "Sending the response failed");
				CAM_MEM_FREE(response.res_msg);
			}
		}
	}
	mutex_unlock(&handle->comms_lock);
}

static void cam_qrtr_sock_data_ready(struct sock *sk)
{
	struct cam_inter_vm_comms_handle *handle = sk->sk_user_data;

	/* The 'handle' becomes NULL when the connection is terminated */
	if (unlikely(!handle)) {
		CAM_ERR(CAM_VMRM, "Invalid comms handle");
		return;
	}

	CAM_DBG(CAM_VMRM, "Submit an offline work to process the message");
	queue_work(handle->msg_recv_wq, &handle->msg_recv_work);
}

int cam_qrtr_initialize_connection(void **hdl, size_t msg_size,
	handle_message_cb msg_cb, bool is_server_vm)
{
	struct cam_qrtr_comms_data *qrtr_data;
	int ret;
	struct qrtr_ctrl_pkt pkt;
	struct msghdr msg = { };
	struct kvec iv = { &pkt, sizeof(pkt) };
	struct cam_inter_vm_comms_handle *handle;

	if (!hdl) {
		CAM_ERR(CAM_VMRM, "Invalid handle holder");
		return -EINVAL;
	}

	handle = CAM_MEM_ZALLOC(sizeof(struct cam_inter_vm_comms_handle), GFP_KERNEL);
	if (unlikely(!handle)) {
		CAM_ERR(CAM_VMRM, "Failed allocating comms handle");
		return -ENOMEM;
	}

	/* Receive buffer should be able to hold control packet as well */
	if (msg_size < sizeof(struct qrtr_ctrl_pkt))
		msg_size = sizeof(struct qrtr_ctrl_pkt);

	handle->msg_size = msg_size;
	handle->msg_buffer = CAM_MEM_ZALLOC(msg_size, GFP_KERNEL);
	if (!handle->msg_buffer) {
		CAM_ERR(CAM_VMRM, "Failed to allocate the memory for comms data");
		CAM_MEM_FREE(handle);
		return -ENOMEM;
	}

	qrtr_data = CAM_MEM_ZALLOC(sizeof(struct cam_qrtr_comms_data), GFP_KERNEL);
	if (!qrtr_data) {
		CAM_ERR(CAM_VMRM, "Failed to allocate the memory for comms data");
		CAM_MEM_FREE(handle->msg_buffer);
		CAM_MEM_FREE(handle);
		return -ENOMEM;
	}
	handle->comms_protocol_data = (void *) qrtr_data;

	INIT_WORK(&handle->msg_recv_work, cam_qrtr_handle_incoming_message);
	handle->msg_recv_wq = alloc_workqueue(CAM_INTER_VM_COMMS_WQ_NAME,
		WQ_HIGHPRI | WQ_UNBOUND, CAM_INTER_VM_COMMS_MAX_PENDING_WORKS);
	if (!handle->msg_recv_wq) {
		CAM_ERR(CAM_VMRM, "Failed to allocate the work queue");
		ret = -ENOMEM;
		goto wq_alloc_failed;
	}

	mutex_init(&handle->comms_lock);
	handle->message_cb = msg_cb;
	handle->is_server_vm = is_server_vm;
	handle->is_comms_established = false;
	handle->is_comms_terminated = false;

	/* Create the socket */
	ret = sock_create_kern(&init_net, AF_QIPCRTR, SOCK_DGRAM,
		PF_QIPCRTR, &qrtr_data->sock);
	if (ret < 0) {
		CAM_ERR(CAM_VMRM, "Failed to create the socket");
		goto early_init_failed;
	}

	ret = kernel_getsockname(qrtr_data->sock, (struct sockaddr *)&qrtr_data->local_sock_addr);
	if (ret < 0) {
		CAM_ERR(CAM_VMRM, "Failed to get socket name");
		goto late_init_failed;
	}

	qrtr_data->sock->sk->sk_user_data = handle;
	qrtr_data->sock->sk->sk_data_ready = cam_qrtr_sock_data_ready;
	qrtr_data->sock->sk->sk_error_report = cam_qrtr_sock_data_ready;

	if (!handle->is_server_vm) {
		/* Send the lookup */
		memset(&pkt, 0, sizeof(pkt));
		pkt.cmd = cpu_to_le32(QRTR_TYPE_NEW_LOOKUP);
		pkt.server.service = cpu_to_le32(CAM_HOST_SERVICE_SVC_ID);
		pkt.server.instance = cpu_to_le32(CAM_HOST_SVC_VERS | CAM_HOST_SERVICE_INS_ID << 8);

		qrtr_data->local_sock_addr.sq_port = QRTR_PORT_CTRL;

		msg.msg_name = &qrtr_data->local_sock_addr;
		msg.msg_namelen = sizeof(qrtr_data->local_sock_addr);

		ret = kernel_sendmsg(qrtr_data->sock, &msg, &iv, 1, sizeof(pkt));
		if (ret < 0) {
			CAM_ERR(CAM_VMRM,
				"Failed to send lookup registration. Init failed: %d", ret);
			goto late_init_failed;
		}
		CAM_DBG(CAM_VMRM, "Server look up has been sent");
	} else {
		/* Start the server */
		memset(&pkt, 0, sizeof(pkt));
		pkt.cmd = cpu_to_le32(QRTR_TYPE_NEW_SERVER);
		pkt.server.service = cpu_to_le32(CAM_HOST_SERVICE_SVC_ID);
		pkt.server.instance = cpu_to_le32(CAM_HOST_SVC_VERS | CAM_HOST_SERVICE_INS_ID << 8);
		pkt.server.node = cpu_to_le32(qrtr_data->local_sock_addr.sq_node);
		pkt.server.port = cpu_to_le32(qrtr_data->local_sock_addr.sq_port);

		qrtr_data->local_sock_addr.sq_port = QRTR_PORT_CTRL;

		msg.msg_name = &qrtr_data->local_sock_addr;
		msg.msg_namelen = sizeof(qrtr_data->local_sock_addr);

		ret = kernel_sendmsg(qrtr_data->sock, &msg, &iv, 1, sizeof(pkt));
		if (ret < 0) {
			CAM_ERR(CAM_VMRM, "Failed to register the service. Init failed: %d", ret);
			goto late_init_failed;
		}
		CAM_DBG(CAM_VMRM, "New server information has been broadcasted");
	}

	*hdl = (void *)handle;
	return 0;

late_init_failed:
	sock_release(qrtr_data->sock);
early_init_failed:
	destroy_workqueue(handle->msg_recv_wq);
wq_alloc_failed:
	CAM_MEM_FREE(handle->msg_buffer);
	CAM_MEM_FREE(handle->comms_protocol_data);
	CAM_MEM_FREE(handle);
	return ret;
}

int cam_qrtr_send_message(void *hdl, void *data, size_t size)
{
	struct cam_inter_vm_comms_handle *handle = (struct cam_inter_vm_comms_handle *)hdl;
	int ret;

	if (unlikely(!handle)) {
		CAM_ERR(CAM_VMRM, "Invalid comms handle");
		return -EINVAL;
	}

	mutex_lock(&handle->comms_lock);

	if (handle->is_comms_terminated) {
		CAM_ERR(CAM_VMRM, "Connection nas been terminated");
		mutex_unlock(&handle->comms_lock);
		return -EINVAL;
	} else if (!handle->is_comms_established) {
		/* Server port (PVM) is expected to start before the Client port (SVM) */
		if (handle->is_server_vm) {
			CAM_WARN(CAM_VMRM, "Client VM has not started the comms yet");
			mutex_unlock(&handle->comms_lock);
			return -EAGAIN;
		}
		CAM_ERR(CAM_VMRM, "Server on the PVM has not started yet");
		mutex_unlock(&handle->comms_lock);
		return -EINVAL;
	}


	ret = cam_qrtr_send_message_locked(handle, data, size);
	if (ret < 0) {
		CAM_ERR(CAM_VMRM, "Failed to send message: %d\n", ret);
		mutex_unlock(&handle->comms_lock);
		return ret;
	}
	CAM_DBG(CAM_VMRM, "The message is successfully sent");

	mutex_unlock(&handle->comms_lock);
	return 0;
}

int cam_qrtr_terminate_connection(void *hdl)
{
	struct cam_qrtr_comms_data *qrtr_data;
	struct cam_inter_vm_comms_handle *handle = (struct cam_inter_vm_comms_handle *)hdl;

	if (unlikely(!handle)) {
		CAM_ERR(CAM_VMRM, "Invalid comms handle");
		return -EINVAL;
	}

	/*
	 * This needs to be refactored when we support graceful
	 * TVM shutdown or the PVM camera server shutdown.
	 */
	mutex_lock(&handle->comms_lock);
	handle->is_comms_terminated = true;

	qrtr_data = (struct cam_qrtr_comms_data *)handle->comms_protocol_data;
	sock_release(qrtr_data->sock);
	qrtr_data->sock = NULL;

	flush_workqueue(handle->msg_recv_wq);
	destroy_workqueue(handle->msg_recv_wq);
	CAM_MEM_FREE(handle->msg_buffer);
	CAM_MEM_FREE(handle->comms_protocol_data);
	mutex_unlock(&handle->comms_lock);
	CAM_MEM_FREE(handle);

	return 0;
}

int cam_get_inter_vm_comms_function_table(struct cam_inter_vm_comms_ops *ops_table)
{
	if (unlikely(!ops_table)) {
		CAM_ERR(CAM_VMRM, "Invalid ops table");
		return -EINVAL;
	}

	ops_table->init = cam_qrtr_initialize_connection;
	ops_table->send_message = cam_qrtr_send_message;
	ops_table->deinit = cam_qrtr_terminate_connection;

	return 0;
}
EXPORT_SYMBOL(cam_get_inter_vm_comms_function_table);
