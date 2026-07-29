// SPDX-License-Identifier: GPL-2.0-only

/* Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/export.h>
#include <linux/skbuff.h>
#include <linux/selinux_netlink.h>
#include <linux/ratelimit.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>
#include <net/netlink.h>
#include "sea_netlink.h"
#include "sea_signal.h"
#include "security.h"

#define MAX_MESSAGES_PER_SEC (30)
#define SEA_SUCCESS (0)

static int sea_daemon_callback(struct sk_buff *skb, struct genl_info *info);
static atomic_t daemon_ready = ATOMIC_INIT(0);

static struct nla_policy sea_netlink_policy[SEA_ATTR_COUNT + 1] = {
	[SEA_VALUE] = { .type = NLA_U64 },
	[SEA_FEATURE_CODE] = { .type = NLA_STRING,
			       .len = FEATURE_CODE_LENGTH + 1 },
	[SEA_DETAIL] = { .type = NLA_STRING,
			 .len = MAX_ALLOWED_DETAIL_LENGTH + 1 },
	[SEA_DAEMON_READY] = { .type = NLA_U32 },
};

static const struct genl_ops sea_kernel_ops[] = {
	{
		.cmd = SEA_MSG_CMD,
		.doit = sea_daemon_callback,
	},
};

struct genl_multicast_group sea_group[] = {
	{
		.name = SEA_GROUP,
	},
};

static struct genl_family sea_family = {
	.name = SEA_FAMILY,
	.version = 1,
	.maxattr = SEA_ATTR_MAX,
	.module = THIS_MODULE,
	.ops = sea_kernel_ops,
	.policy = sea_netlink_policy,
	.mcgrps = sea_group,
	.n_mcgrps = ARRAY_SIZE(sea_group),
	.n_ops = ARRAY_SIZE(sea_kernel_ops),
};

int __init sea_netlink_init(void)
{
	int ret;

	ret = genl_register_family(&sea_family);
	if (ret != SEA_SUCCESS)
		pr_err("Netlink register failed: %d.", ret);

	return ret;
}

void __exit sea_netlink_exit(void)
{
	int ret;

	ret = genl_unregister_family(&sea_family);
	if (ret != SEA_SUCCESS)
		pr_err("Netlink unregister failed: %d.", ret);
}

static int sea_daemon_callback(struct sk_buff *skb, struct genl_info *info)
{
	if (atomic_add_unless(&daemon_ready, 1, 1))
		pr_info("ngk_security_audit daemon ready");

	return SEA_SUCCESS;
}

int sea_daemon_ready(void)
{
	return atomic_read(&daemon_ready);
}

int sea_send_netlink_message(const char *feature_code, const char *detail,
			     int64_t value)
{
	int ret;
	void *msg_head;
	size_t detail_len;
	struct sk_buff *skb;

	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (skb == NULL) {
		pr_err("genlmsg_new error.");
		return -ENOMEM;
	}

	msg_head = genlmsg_put(skb, 0, 0, &sea_family, 0, SEA_MSG_CMD);
	if (msg_head == NULL) {
		pr_err("genlmsg_put error.");
		nlmsg_free(skb);
		return -ENOMEM;
	}

	ret = nla_put(skb, SEA_VALUE, sizeof(value), &value);
	if (ret) {
		pr_err("nla_put value error.");
		nlmsg_free(skb);
		return ret;
	}

	ret = nla_put(skb, SEA_FEATURE_CODE, FEATURE_CODE_LENGTH + 1,
		      feature_code);
	if (ret) {
		pr_err("nla_put feature error.");
		nlmsg_free(skb);
		return ret;
	}

	detail_len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);
	ret = nla_put(skb, SEA_DETAIL, detail_len + 1, detail);
	if (ret) {
		pr_err("nla_put detail error.");
		nlmsg_free(skb);
		return ret;
	}

	genlmsg_end(skb, msg_head);
	ret = genlmsg_multicast(&sea_family, skb, 0, 0, GFP_ATOMIC);
	if (ret) {
		pr_err("genlmsg_multicast error.");
		return ret;
	}

	return SEA_SUCCESS;
}

int sea_check_message_rate_limit(void)
{
	static DEFINE_RATELIMIT_STATE(sea_ratelimit_state, 1 * HZ,
				      MAX_MESSAGES_PER_SEC);

	if (__ratelimit(&sea_ratelimit_state))
		return SEA_SUCCESS;

	pr_err("SEA check message rate limit error.");
	return -EBUSY;
}

noinline int sea_send_message(const char *feature_code, const char *detail,
			      int64_t value)
{
	int ret;
	size_t len;

	if (!sea_daemon_ready()) {
		pr_err("SEA daemon not ready.");
		ret = -EPERM;
		goto exit_send;
	}

	if (!is_sea_socket_initialized) {
		pr_err("SEA socket not initialized.");
		ret = -EACCES;
		goto exit_send;
	}

	if (!feature_code) {
		pr_err("Invalid feature code.");
		ret = -EINVAL;
		goto exit_send;
	}

	if (!detail)
		detail = "";

	len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);

	ret = sea_check_message_rate_limit();
	if (ret != SEA_SUCCESS)
		goto exit_send;

	ret = sea_send_netlink_message(feature_code, detail, value);
	if (ret == SEA_SUCCESS) {
		pr_info("Send SEA netlink message success - {'%s', '%s' (%zu bytes), %lld}",
			feature_code, detail, len, value);
	}

exit_send:
	return ret;
}
