// SPDX-License-Identifier: GPL-2.0
//
// In Samsung R&D Institute Ukraine, LLC (SRUKR) under a contract between
// Samsung R&D Institute Ukraine, LLC (Kyiv, Ukraine)
// and "Samsung Electronics Co", Ltd (Seoul, Republic of Korea)
// Copyright: (c) Samsung Electronics Co, Ltd 2024. All rights reserved.
//

#include <linux/kernel.h>
#include <crypto/hash.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/crypto.h>

#include "fips140.h"
#include "fips140_3_services.h"
#include "fips140_3_services_internal.h"

static const char * const approved_algs[] = {
	"cbc(aes-generic)",
	"ecb(aes-generic)",
	"hmac(sha1-generic)",
	"hmac(sha224-generic)",
	"hmac(sha256-generic)",
	"hmac(sha384-generic)",
	"hmac(sha512-generic)",
	"sha1-generic",
	"sha224-generic",
	"sha256-generic",
	"sha384-generic",
	"sha512-generic",
	"ecb(aes-ce)",
	"cbc(aes-ce)",
	"hmac(sha1-ce)",
	"hmac(sha224-ce)",
	"hmac(sha256-ce)",
	"sha1-ce",
	"sha224-ce",
	"sha256-ce",
};

uint32_t skc_is_approved_service(const char *alg_name)
{
	size_t i = 0;

	for (i = 0; i < ARRAY_SIZE(approved_algs); ++i) {
		if (!strcmp(alg_name, approved_algs[i]))
			return 1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(skc_is_approved_service);

const char *skc_module_get_version(void)
{
	return SKC_VERSION_TEXT;
}
EXPORT_SYMBOL_GPL(skc_module_get_version);

struct tfm_func {
	const void *(*get_tfm)(const void *data);
	void (*final_tfm_approve_status)(void *data);
	void (*clean_tfm_approve_status)(void *data);
	int (*get_tfm_approve_status)(const void *data);
};

/* Data structures for general crypto */

struct general_data {
	const void *tfm;
	char crypto_id[CRYPTO_MAX_ALG_NAME];
	int service_final_status;
};

static int get_general_approve_status(const void *data);
static const void *get_general_tfm(const void *data);
static void crypto_general_clean_approve_status(void *data);

static const struct tfm_func general_tfm_f = {
	.get_tfm = get_general_tfm,
	.clean_tfm_approve_status = crypto_general_clean_approve_status,
	.final_tfm_approve_status = NULL,
	.get_tfm_approve_status = get_general_approve_status,
};

/* Data structures for HMAC crypto */

#define FIPS_HMAC_MIN_KEY_SIZE_BYTES (14)

struct hmac_data {
	const void *tfm;
	char crypto_id[CRYPTO_MAX_ALG_NAME];
	uint32_t service_hash_by_key_status;
	int service_final_status;
};

static const void *get_hmac_tfm(const void *data);
static void crypto_hmac_clean_approve_status(void *data);
static void crypto_hmac_final_approve_status(void *data);
static int get_hmac_approve_status(const void *data);


static const struct tfm_func hmac_f = {
	.get_tfm = get_hmac_tfm,
	.clean_tfm_approve_status = crypto_hmac_clean_approve_status,
	.final_tfm_approve_status = crypto_hmac_final_approve_status,
	.get_tfm_approve_status = get_hmac_approve_status,
};

/* Indicator properties storage harness */

struct service_table_node {
	void *tfm_data;
	const struct tfm_func *tfm_func;
	struct list_head list;
};

static LIST_HEAD(service_table);
static DEFINE_SPINLOCK(service_table_lock);

#define LOCK_SERVICE_TABLE(flag)	spin_lock_irqsave(&service_table_lock, flag)
#define UNLOCK_SERVICE_TABLE(flag)	spin_unlock_irqrestore(&service_table_lock, flag)

static struct service_table_node *service_table_add_node(struct service_table_node *node)
{
	list_add_tail(&node->list, &service_table);
	return node;
}

static void service_table_remove_node(struct service_table_node *node)
{
	list_del(&(node->list));
	kfree_sensitive(node);
}

static struct service_table_node *service_table_get_item(const void *tfm)
{
	struct service_table_node *res = NULL;
	struct service_table_node *temp_node = NULL;

	list_for_each_entry(temp_node, &service_table, list) {
		if (temp_node->tfm_func->get_tfm &&
			tfm == temp_node->tfm_func->get_tfm(temp_node->tfm_data)) {
			res = temp_node;
			goto exit;
		}
	}

exit:
	return res;
}

/* Indicator API implementation for associated crypto */

static int prepare_tfm_support_str(const char *crypto_id, const void *tfm,
				const void **func, void **data, gfp_t flags)
{
	int res = 0;

	/* Currently, only HMAC needs check for runtime params, so make it simple. */
	if (strstr(crypto_id, "hmac") == crypto_id) {
		*data = kzalloc(sizeof(struct hmac_data), flags);
		if (!*data) {
			res = -ENOMEM;
			goto err;
		}
		((struct hmac_data *)(*data))->tfm = tfm;
		((struct hmac_data *)(*data))->service_hash_by_key_status = STATUS_UNDEFINED;
		((struct hmac_data *)(*data))->service_final_status = STATUS_UNDEFINED;
		strscpy(((struct hmac_data *)(*data))->crypto_id, crypto_id,
			CRYPTO_MAX_ALG_NAME);
		*func = &hmac_f;
	} else {
		*data = kzalloc(sizeof(struct general_data), flags);
		if (!*data) {
			res = -ENOMEM;
			goto err;
		}
		((struct general_data *)(*data))->tfm = tfm;
		((struct general_data *)(*data))->service_final_status = STATUS_UNDEFINED;
		strscpy(((struct general_data *)(*data))->crypto_id, crypto_id,
			CRYPTO_MAX_ALG_NAME);
		*func = &general_tfm_f;
	}

err:
	return res;
}

int init_service_indicator(const void *tfm, const char *crypto_id, gfp_t flags)
{
	struct service_table_node *node = NULL;
	void *tfm_serv_data = NULL;
	const void *tfm_serv_func = NULL;
	int res = -EINVAL;
	unsigned long lock_flags;

	if (!tfm
		|| !crypto_id
		|| (strnlen(crypto_id, CRYPTO_MAX_ALG_NAME) > (CRYPTO_MAX_ALG_NAME - 1)))
		return -EINVAL;

	/* If the property item exists in the list already. */
	LOCK_SERVICE_TABLE(lock_flags);
	node = service_table_get_item(tfm);
	if (node) {
		node->tfm_func->clean_tfm_approve_status(node->tfm_data);
		UNLOCK_SERVICE_TABLE(lock_flags);
		return 0;
	}
	UNLOCK_SERVICE_TABLE(lock_flags);

	/* If the new property has to be added into the list. */
	res = prepare_tfm_support_str(crypto_id, tfm, &tfm_serv_func, &tfm_serv_data, flags);
	if (res)
		return res;

	node = kzalloc(sizeof(struct service_table_node), flags);
	if (!node) {
		if (tfm_serv_data)
			kfree_sensitive(tfm_serv_data);
		return -ENOMEM;
	}

	LOCK_SERVICE_TABLE(lock_flags);
	node = service_table_add_node(node);
	node->tfm_data = tfm_serv_data;
	node->tfm_func = tfm_serv_func;
	UNLOCK_SERVICE_TABLE(lock_flags);

	return 0;
}
EXPORT_SYMBOL_GPL(init_service_indicator);

int get_service_status(const void *tfm)
{
	int res = -EINVAL;
	const struct service_table_node *node = NULL;
	unsigned long lock_flags;

	LOCK_SERVICE_TABLE(lock_flags);
	node = service_table_get_item(tfm);
	if (!node)
		goto exit;

	if (node->tfm_func->get_tfm_approve_status)
		res = node->tfm_func->get_tfm_approve_status(node->tfm_data);

	if (res == STATUS_UNDEFINED)
		res = -EINVAL;

exit:
	UNLOCK_SERVICE_TABLE(lock_flags);
	return res;
}
EXPORT_SYMBOL_GPL(get_service_status);

void calc_final_state_service_indicator(const void *tfm)
{
	const struct service_table_node *node = NULL;
	unsigned long lock_flags;

	LOCK_SERVICE_TABLE(lock_flags);

	node = service_table_get_item(tfm);
	if (!node)
		goto exit;

	if (node->tfm_func->final_tfm_approve_status)
		node->tfm_func->final_tfm_approve_status(node->tfm_data);

exit:
	UNLOCK_SERVICE_TABLE(lock_flags);
}
EXPORT_SYMBOL_GPL(calc_final_state_service_indicator);

void finit_service_indicator(const void *tfm)
{
	struct service_table_node *node = NULL;
	unsigned long lock_flags;

	LOCK_SERVICE_TABLE(lock_flags);
	node = service_table_get_item(tfm);
	if (!node)
		goto exit;

	if (node->tfm_data)
		kfree_sensitive(node->tfm_data);

	service_table_remove_node(node);

exit:
	UNLOCK_SERVICE_TABLE(lock_flags);
}
EXPORT_SYMBOL_GPL(finit_service_indicator);

/* HMAC associated functionality */

void crypto_hmac_set_key_approve_status(const void *tfm, unsigned int key_size)
{
	struct service_table_node *node = NULL;
	unsigned long lock_flags;

	LOCK_SERVICE_TABLE(lock_flags);
	node = service_table_get_item(tfm);
	if (!node)
		goto exit;

	((struct hmac_data *)node->tfm_data)->service_hash_by_key_status = STATUS_NON_APPROVED;
	if (key_size >= FIPS_HMAC_MIN_KEY_SIZE_BYTES)
		((struct hmac_data *)node->tfm_data)->service_hash_by_key_status = STATUS_APPROVED;

exit:
	UNLOCK_SERVICE_TABLE(lock_flags);
}

static void crypto_hmac_clean_approve_status(void *data)
{
	((struct hmac_data *)data)->service_final_status = STATUS_UNDEFINED;
}

static void crypto_hmac_final_approve_status(void *data)
{
	struct hmac_data *tfm_data = (struct hmac_data *)data;

	tfm_data->service_final_status = STATUS_NON_APPROVED;
	if ((tfm_data->service_hash_by_key_status == STATUS_APPROVED)
		&& skc_is_approved_service(tfm_data->crypto_id)) {
		tfm_data->service_final_status = STATUS_APPROVED;
	}
}

static int get_hmac_approve_status(const void *data)
{
	return ((struct hmac_data *)data)->service_final_status;
};

static const void *get_hmac_tfm(const void *data)
{
	return ((struct hmac_data *)data)->tfm;
};


/* General crypto Functionality */

static void crypto_general_clean_approve_status(void *data)
{
	((struct general_data *)data)->service_final_status = STATUS_UNDEFINED;
}

static int get_general_approve_status(const void *data)
{
	return skc_is_approved_service(((struct general_data *)data)->crypto_id);
};

static const void *get_general_tfm(const void *data)
{
	return ((struct general_data *)data)->tfm;
};
