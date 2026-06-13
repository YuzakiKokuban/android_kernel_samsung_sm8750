/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/ipc_logging.h>
#include "dfc.h"
#include "rmnet_qmi.h"
#include "rmnet_ctl.h"
#include "rmnet_qmap.h"
#include "rmnet_module.h"
#include "rmnet_hook.h"

static atomic_t qmap_txid;
static void *rmnet_ctl_handle;
static void *rmnet_port;
static struct net_device *real_data_dev;
static struct rmnet_ctl_client_if *rmnet_ctl;

int rmnet_qmap_send(struct sk_buff *skb, u8 ch, bool flush)
{
	trace_dfc_qmap(skb->data, skb->len, false, ch);

	if (ch != RMNET_CH_CTL && real_data_dev) {
		skb->protocol = htons(ETH_P_MAP);
		skb->dev = real_data_dev;
		rmnet_ctl->log(RMNET_CTL_LOG_DEBUG, "TXI", 0, skb->data,
			       skb->len);

		rmnet_map_tx_qmap_cmd(skb, ch, flush);
		return 0;
	}

	if (rmnet_ctl->send(rmnet_ctl_handle, skb)) {
		net_log("Failed to send to rmnet ctl\n");
		return -ECOMM;
	}

	return 0;
}
EXPORT_SYMBOL(rmnet_qmap_send);

static void rmnet_print_packet(const struct sk_buff *skb, char *dir)
{
	struct qmap_cmd_hdr *cmd;
	char buffer[200];
	unsigned int len, printlen;
	int i, buffloc = 0;

	printlen = 48;

	if (skb->len > 0)
		len = skb->len;
	else
		len = ((unsigned int)(uintptr_t)skb->end) -
		      ((unsigned int)(uintptr_t)skb->data);

	cmd = (struct qmap_cmd_hdr *)skb->data;
	net_log("%s() QMAP cmd %d %s-ed type:%d len:%d",
					 __func__, cmd->cmd_name, dir,
					 cmd->cmd_type, len);
	memset(buffer, 0, sizeof(buffer));
	for (i = 0; (i < printlen) && (i < len); i++) {
		if ((i % 16) == 0) {
			net_log("[%s] - PKT%s\n", dir, buffer);
			memset(buffer, 0, sizeof(buffer));
			buffloc = 0;
			buffloc += snprintf(&buffer[buffloc],
					    sizeof(buffer) - buffloc, "%04X:",
					    i);
		}

		buffloc += snprintf(&buffer[buffloc], sizeof(buffer) - buffloc,
					" %02x", skb->data[i]);
	}
	net_log("[%s] - PKT%s\n", dir, buffer);
}

static void rmnet_qmap_cmd_handler(struct sk_buff *skb)
{
	struct qmap_cmd_hdr *cmd;
	int rc = QMAP_CMD_DONE;

	if (!skb)
		return;

	trace_dfc_qmap(skb->data, skb->len, true, RMNET_CH_CTL);

	if (skb->len < sizeof(struct qmap_cmd_hdr))
		goto free_skb;

	cmd = (struct qmap_cmd_hdr *)skb->data;
	if (!cmd->cd_bit || skb->len != ntohs(cmd->pkt_len) + QMAP_HDR_LEN)
		goto free_skb;

	rcu_read_lock();

	switch (cmd->cmd_name) {
	case QMAP_DFC_CONFIG:
	case QMAP_DFC_IND:
	case QMAP_DFC_QUERY:
	case QMAP_DFC_END_MARKER:
	case QMAP_DFC_POWERSAVE:
		rmnet_print_packet(skb, "RX");
		rc = dfc_qmap_cmd_handler(skb);
		break;

	case QMAP_LL_SWITCH:
	case QMAP_LL_SWITCH_STATUS:
		rc = ll_qmap_cmd_handler(skb);
		break;

	case QMAP_DATA_REPORT:
		rmnet_module_hook_aps_data_report(skb);
		rc = QMAP_CMD_DONE;
		break;

	case QMAP_CMD_31:
	case QMAP_CMD_32:
	case QMAP_CMD_40:
	case QMAP_CMD_41:
	case QMAP_CMD_42:
		rmnet_module_hook_perf_cmd_ingress(skb);
		rc = QMAP_CMD_DONE;
		break;

	default:
		if (cmd->cmd_type == QMAP_CMD_REQUEST)
			rc = QMAP_CMD_UNSUPPORTED;
	}

	/* Send ack */
	if (rc != QMAP_CMD_DONE) {
		if (rc == QMAP_CMD_ACK_INBAND) {
			cmd->cmd_type = QMAP_CMD_ACK;
			rmnet_qmap_send(skb, RMNET_CH_DEFAULT, false);
		} else {
			cmd->cmd_type = rc;
			rmnet_print_packet(skb, "TX");
			rmnet_qmap_send(skb, RMNET_CH_CTL, false);
		}
		rcu_read_unlock();
		return;
	}

	rcu_read_unlock();

free_skb:
	kfree_skb(skb);
}

static struct rmnet_ctl_client_hooks cb = {
	.ctl_dl_client_hook = rmnet_qmap_cmd_handler,
};

int rmnet_qmap_next_txid(void)
{
	return atomic_inc_return(&qmap_txid);
}
EXPORT_SYMBOL(rmnet_qmap_next_txid);

struct net_device *rmnet_qmap_get_dev(u8 mux_id)
{
	return rmnet_get_rmnet_dev(rmnet_port, mux_id);
}

int rmnet_qmap_init(void *port)
{
	if (rmnet_ctl_handle)
		return 0;

	atomic_set(&qmap_txid, 0);
	rmnet_port = port;
	real_data_dev = rmnet_get_real_dev(rmnet_port);

	rmnet_ctl = rmnet_ctl_if();
	if (!rmnet_ctl) {
		net_log("rmnet_ctl module not loaded\n");
		return -EFAULT;
	}

	if (rmnet_ctl->reg)
		rmnet_ctl_handle = rmnet_ctl->reg(&cb);

	if (!rmnet_ctl_handle) {
		net_log("Failed to register with rmnet ctl\n");
		return -EFAULT;
	}

	return 0;
}

void rmnet_qmap_exit(void)
{
	int rc = 0;

	if (rmnet_ctl && rmnet_ctl->dereg) {
		rc = rmnet_ctl->dereg(rmnet_ctl_handle);
		net_log("%s() deregistered  with rmnet ctl: %d\n", __func__, rc);
	}

	rmnet_ctl_handle = NULL;
	real_data_dev = NULL;
	rmnet_port = NULL;
}
