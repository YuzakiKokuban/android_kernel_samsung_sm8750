// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <security.h>
#include "sea_signal.h"
#include "sea_hash.h"
#include "sea_netlink.h"

static int sea_signal_state;
static atomic_t is_permissive_audit = ATOMIC_INIT(0);
static atomic_t is_domain_permissive_audit = ATOMIC_INIT(0);
int is_sea_socket_initialized;

// Sysfs show/store functions for the 'toggle' attribute
static ssize_t sea_signal_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	pr_info("%s: %d\n", __func__, sea_signal_state);
	return sprintf(buf, "%d\n", sea_signal_state);
}

static ssize_t sea_signal_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf,
				size_t count)
{
	int ret;

	pr_info("%s from: %d\n", __func__, sea_signal_state);
	ret = kstrtoint(buf, 0, &sea_signal_state);
	if (ret)
		return ret;
	pr_info("%s to: %d\n", __func__, sea_signal_state);
	return count;
}

// Define the sysfs attribute
static struct kobj_attribute sea_signal_attr =
	__ATTR(sea_enabled, 0664, sea_signal_show, sea_signal_store);

// Global kobject instance
static struct kobject *sea_kobj;

bool sea_signal_enabled(void)
{
	return sea_signal_state == 1;
}

// Module initialization
int __init sea_signal_init(void)
{
	int ret;
	sea_signal_state = 0;
	is_sea_socket_initialized = false;

	if (!kernel_kobj) {
		pr_info("Kernel Kobject does not exist\n");
		return -ENOMEM;
	}

	// Create and register the kobject under /sys/kernel/
	sea_kobj = kobject_create_and_add("sec_sea", kernel_kobj);
	if (!sea_kobj)
		return -ENOMEM;

	// Register the sysfs attribute
	if (sysfs_create_file(sea_kobj, &sea_signal_attr.attr)) {
		kobject_put(sea_kobj);
		return -EIO;
	}

	pr_info("Kobject 'sec_sea' created at /sys/kernel/sec_sea\n");

	ret = sea_netlink_init();

	if (ret != 0) {
		pr_err("Failed to initialize SEA netlink socket");
		return -1;
	}
	is_sea_socket_initialized = true;

	return 0;
}

void sea_detect_sensitive_domain(u32 ssid, u32 tsid, u32 requested,
				 struct av_decision *avd)
{
	int ret = 1;

	if (sea_signal_enabled() && atomic_read(&is_permissive_audit) == 0) {
		char *sc, *tc;
		int rc1, rc2;
		u32 sc_len, tc_len, denied;
		char detail[1024] = {
			0,
		};
		int detail_size = 0;

		denied = requested & ~(avd->allowed);
		rc1 = security_sid_to_context(ssid, &sc, &sc_len);
		rc2 = security_sid_to_context(tsid, &tc, &tc_len);
		if (!rc2) {
			if (sea_search(tc)) {
				pr_err("SEA : Sensitive domain access detected - key_hash : %s present\n",
				       tc);
				pr_err("SEA : scontext=%s tcontext=%s\n", sc,
				       tc);
				detail_size = snprintf(
					detail, sizeof(detail),
					"Sensitive domain access detected ");
				if (!rc1)
					detail_size +=
						snprintf(detail + detail_size,
							 sizeof(detail) - detail_size,
							 "scontext=%s ", sc);
				else
					detail_size += snprintf(
						detail + detail_size,
						sizeof(detail) - detail_size,
						"scontext(ssid)=%d ", ssid);
				snprintf(detail + detail_size, sizeof(detail) - detail_size,
					 "tcontext=%s ", tc);

				pr_warn("SEA : Sending SEANDROID_EVENT (DD) Signal to NGK");
				ret = sea_send_message("SEA_DD", detail, 0);
				pr_warn("SEA : Send message return value = %d\n",
					ret);
			}
		}
		if (!rc1)
			kfree(sc);
		if (!rc2)
			kfree(tc);
	}
}

void sea_detect_permissive(u32 ssid, struct av_decision *avd)
{
	int ret = 1;

	if (sea_signal_enabled()) {
		if (!enforcing_enabled()) {
			if (atomic_cmpxchg(&is_permissive_audit, 0, 1) == 0) {
				pr_warn("SEA : Sending SEANDROID_EVENT (PD) Signal to NGK");
				ret = sea_send_message(
					"SEA_PD", "Permissive mode detected",
					0);
				pr_warn("SEA : Send message return value = %d\n",
					ret);
				if (ret != 0)
					atomic_set(&is_permissive_audit, 0);
			}
		}

		if ((avd->flags & AVD_FLAGS_PERMISSIVE)) {
			if (atomic_cmpxchg(&is_domain_permissive_audit, 0, 1) == 0) {
				char *sc;
				int rc;
				u32 sc_len;
				char detail[201] = {
					0,
				};

				rc = security_sid_to_context(ssid, &sc,
							     &sc_len);
				if (rc)
					snprintf(
						detail, sizeof(detail),
						"Permissive domain detected, ssid=%d",
						ssid);
				else
					snprintf(
						detail, sizeof(detail),
						"Permissive domain detected, scontext=%s",
						sc);

				pr_warn("SEA : Sending SEANDROID_EVENT (PDD) Signal to NGK");
				ret = sea_send_message("SEA_PDD", detail, 0);
				pr_warn("SEA : Send message return value = %d\n",
					ret);
				if (ret != 0)
					atomic_set(&is_domain_permissive_audit, 0);

				if (!rc)
					kfree(sc);
			}
		}
	}
}

// Module cleanup
void __exit sea_signal_exit(void)
{
	if (is_sea_socket_initialized) {
		is_sea_socket_initialized = false;
		sea_netlink_exit();
	}
	// Remove the sysfs attribute and release the kobject
	sysfs_remove_file(sea_kobj, &sea_signal_attr.attr);
	kobject_put(sea_kobj);
	pr_info("Kobject 'sec_sea' removed\n");
}

module_init(sea_signal_init);
module_exit(sea_signal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Avijit Borah <avijit.borah@samsung.com>");
MODULE_AUTHOR("Suraj Kishor Thakre <suraj.t@samsung.com>");
