/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/ngksm.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/version.h>
#include <net/genetlink.h>
#include "ngksm_netlink.h"
#include "ngksm_common.h"
#include "ngk_hypervisor_detector.h"


static int ngksm_daemon_callback(struct sk_buff *skb,
					       struct genl_info *info);
static atomic_t daemon_ready = ATOMIC_INIT(0);

static struct nla_policy ngksm_netlink_policy[NGKSM_ATTR_COUNT + 1] = {
	[NGKSM_VALUE] = { .type = NLA_U64 },
	[NGKSM_FEATURE_CODE] = { .type = NLA_STRING, .len = FEATURE_CODE_LENGTH + 1},
	[NGKSM_DETAIL] = { .type = NLA_STRING, .len = MAX_ALLOWED_DETAIL_LENGTH + 1},
	[NGKSM_DAEMON_READY] = { .type = NLA_U32 },
};

static const struct genl_ops ngksm_kernel_ops[] = {
	{
		.cmd = NGKSM_MSG_CMD,
		.doit = ngksm_daemon_callback,
	},
};

struct genl_multicast_group ngksm_group[] = {
	{
		.name = NGKSM_GROUP,
	},
};

static struct genl_family ngksm_family = {
	.name = NGKSM_FAMILY,
	.version = 1,
	.maxattr = NGKSM_ATTR_MAX,
	.module = THIS_MODULE,
	.ops = ngksm_kernel_ops,
	.policy = ngksm_netlink_policy,
	.mcgrps = ngksm_group,
	.n_mcgrps = ARRAY_SIZE(ngksm_group),
	.n_ops = ARRAY_SIZE(ngksm_kernel_ops),
};

int __init ngksm_netlink_init(void)
{
	int ret;

	ret = genl_register_family(&ngksm_family);
	if (ret != NGKSM_SUCCESS)
		NGKSM_LOG_ERROR("Netlink register failed: %d.", ret);

	return ret;
}

void __exit ngksm_netlink_exit(void)
{
	int ret;

	ret = genl_unregister_family(&ngksm_family);
	if (ret != NGKSM_SUCCESS)
		NGKSM_LOG_ERROR("Netlink unregister failed: %d.", ret);
}

static int ngksm_daemon_callback(struct sk_buff *skb,
					       struct genl_info *info)
{
	if (atomic_add_unless(&daemon_ready, 1, 1)) {
		NGKSM_LOG_INFO("ngk_security_audit daemon ready");
//		cmd_hyp_detector(1023);
	}
	return NGKSM_SUCCESS;
}

int ngksm_daemon_ready(void)
{
	return atomic_read(&daemon_ready);
}

int ngksm_send_netlink_message(const char *feature_code,
			      const char *detail,
			      int64_t value)
{
	int ret;
	void *msg_head;
	size_t detail_len;
	struct sk_buff *skb;

	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (skb == NULL) {
		NGKSM_LOG_ERROR("genlmsg_new error.");
		return -ENOMEM;
	}

	msg_head = genlmsg_put(skb, 0, 0,
			       &ngksm_family, 0, NGKSM_MSG_CMD);
	if (msg_head == NULL) {
		NGKSM_LOG_ERROR("genlmsg_put error.");
		nlmsg_free(skb);
		return -ENOMEM;
	}

	ret = nla_put(skb, NGKSM_VALUE, sizeof(value), &value);
	if (ret) {
		NGKSM_LOG_ERROR("nla_put value error.");
		nlmsg_free(skb);
		return ret;
	}

	ret = nla_put(skb, NGKSM_FEATURE_CODE,
		      FEATURE_CODE_LENGTH + 1, feature_code);
	if (ret) {
		NGKSM_LOG_ERROR("nla_put feature error.");
		nlmsg_free(skb);
		return ret;
	}

	detail_len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);
	ret = nla_put(skb, NGKSM_DETAIL, detail_len + 1, detail);
	if (ret) {
		NGKSM_LOG_ERROR("nla_put detail error.");
		nlmsg_free(skb);
		return ret;
	}

	genlmsg_end(skb, msg_head);
	ret = genlmsg_multicast(&ngksm_family, skb, 0, 0, GFP_ATOMIC);
	if (ret) {
		NGKSM_LOG_ERROR("genlmsg_multicast error.");
		return ret;
	}

	return NGKSM_SUCCESS;
}
