// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA keyring
 *
 * Copyright (C) 2023 Samsung Electronics, Inc.
 * Oleksandr Stanislavskyi, <o.stanislavs@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/key-type.h>
#include <crypto/public_key.h>
#include <crypto/hash_info.h>
#include <keys/asymmetric-type.h>
#include <linux/key.h>
#include <linux/cred.h>

#include "proca_certificate_db.h"
#include "proca_log.h"
#include "proca_porting.h"

static struct key *proca_keyring;
static const char *proca_keyring_name = "_proca";

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
static inline int proca_verify_signature(struct key *key,
			  struct public_key_signature *pks,
			  const char *signature, int sig_len)
{
	int ret = -ENOMEM;

	pks->hash_algo = hash_algo_name[HASH_ALGO_SHA256];
	pks->nr_mpi = 1;
	pks->rsa.s = mpi_read_raw_data(signature, sig_len);

	if (pks->rsa.s)
		ret = verify_signature(key, pks);

	mpi_free(pks->rsa.s);

	return ret;
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
static inline int proca_verify_signature(struct key *key,
			  struct public_key_signature *pks,
			  const char *signature, int sig_len)
{
	int ret = -ENOMEM;

	pks->pkey_algo = "rsa";
	pks->hash_algo = hash_algo_name[HASH_ALGO_SHA256];
	pks->s = (u8 *)signature;
	pks->s_size = sig_len;
	ret = verify_signature(key, pks);

	return ret;
}
#else
static inline int proca_verify_signature(struct key *key,
			  struct public_key_signature *pks,
			  const char *signature, int sig_len)
{
	int ret = -ENOMEM;

	pks->pkey_algo = "rsa";
	pks->encoding = "pkcs1";
	pks->hash_algo = hash_algo_name[HASH_ALGO_SHA256];
	pks->s = (u8 *)signature;
	pks->s_size = sig_len;
	ret = verify_signature(key, pks);

	return ret;
}
#endif

static struct key *proca_request_asymmetric_key(uint32_t keyid)
{
	struct key *key;
	char name[12];

	snprintf(name, sizeof(name), "id:%08x", keyid);

	PROCA_DEBUG_LOG("key search: \"%s\"\n", name);

	if (proca_keyring) {
		/* search in specific keyring */
		key_ref_t kref;

		kref = keyring_search(make_key_ref(proca_keyring, 1),
			&key_type_asymmetric, name, true);
		if (IS_ERR(kref)) {

			key = ERR_CAST(kref);
			PROCA_ERROR_LOG("keyring search error, %lx", PTR_ERR(key));
		}
		else
			key = key_ref_to_ptr(kref);
	} else {
		PROCA_ERROR_LOG("error: no keyring\n");
		return ERR_PTR(-ENOKEY);
	}

	if (IS_ERR(key)) {
		switch (PTR_ERR(key)) {
			/* Hide some search errors */
		case -EACCES:
		case -ENOTDIR:
		case -EAGAIN:
			return ERR_PTR(-ENOKEY);
		default:
			return key;
		}
	}

	PROCA_DEBUG_LOG("%s() = 0 [%x]\n", __func__, key_serial(key));

	return key;
}

static int proca_asymmetric_verify(const char *signature, int sig_len,
				   const char *hash, int hash_len, uint32_t key_id)
{
	struct public_key_signature pks;
	struct key *key;
	int ret = -ENOMEM;

	key = proca_request_asymmetric_key(__be32_to_cpu(key_id));

	memset(&pks, 0, sizeof(pks));

	pks.digest = (u8 *)hash;
	pks.digest_size = hash_len;
	ret = proca_verify_signature(key, &pks, signature, sig_len);
	key_put(key);

	PROCA_DEBUG_LOG("%s() = %d\n", __func__, ret);

	return ret;
}

int proca_digsig_verify(const char *signature, int sig_len,
						const char *hash, int hash_len, uint32_t key_id)
{
	if (!proca_keyring) {
		proca_keyring = request_key(
			&key_type_keyring, proca_keyring_name, NULL);
		if (IS_ERR(proca_keyring)) {
			int err = PTR_ERR(proca_keyring);

			PROCA_ERROR_LOG("no %s keyring: %d\n", proca_keyring_name, err);
			proca_keyring = NULL;
			return err;
		}
	}

	return proca_asymmetric_verify(signature, sig_len, hash, hash_len, key_id);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
static inline struct key *proca_keyring_alloc(const char *description,
			  kuid_t uid, kgid_t gid, const struct cred *cred,
			  key_perm_t perm, unsigned long flags)
{
	return keyring_alloc(description, uid, gid, cred,
				perm, flags, NULL);
}
#else
static inline struct key *proca_keyring_alloc(const char *description,
			  kuid_t uid, kgid_t gid, const struct cred *cred,
			  key_perm_t perm, unsigned long flags)
{
	return keyring_alloc(description, uid, gid, cred,
				perm, flags, NULL, NULL);
}
#endif

int __init proca_load_x509_from_mem(const char *data, size_t size)
{
	key_ref_t key;
	int rc = 0;

	if (!proca_keyring || size == 0)
		return -EINVAL;

	key = key_create_or_update(make_key_ref(proca_keyring, 1),
		"asymmetric",
		NULL,
		data,
		size,
		((KEY_POS_ALL & ~KEY_POS_SETATTR) |
		KEY_USR_VIEW | KEY_USR_READ),
		KEY_ALLOC_NOT_IN_QUOTA);
	if (IS_ERR(key)) {
		rc = PTR_ERR(key);
		PROCA_ERROR_LOG("Problem loading X.509 certificate (%d): %s\n",
			rc, "built-in");
	} else {
		pr_notice("Loaded X.509 cert '%s': %s\n",
		key_ref_to_ptr(key)->description, "built-in");
		key_ref_put(key);
	}

	return rc;
}

#ifdef CONFIG_PROCA_CERT_ENG
extern char proca_local_ca_start_eng[];
extern char proca_local_ca_end_eng[];

int __init proca_import_eng_key(void)
{
	size_t size = proca_local_ca_end_eng - proca_local_ca_start_eng;

	return proca_load_x509_from_mem(proca_local_ca_start_eng, size);
}
#else

int __init proca_import_eng_key(void)
{
	return 0;
}
#endif

#ifdef CONFIG_PROCA_CERT_USER
extern char proca_local_ca_start_user[];
extern char proca_local_ca_end_user[];

int __init proca_import_user_key(void)
{
	size_t size = proca_local_ca_end_user - proca_local_ca_start_user;

	return proca_load_x509_from_mem(proca_local_ca_start_user, size);
}
#else

int __init proca_import_user_key(void)
{
	return 0;
}
#endif

int __init proca_load_built_x509(void)
{
	int rc;

	rc = proca_import_eng_key();
	if (rc)
		return rc;

	rc = proca_import_user_key();

	return rc;
}

int __init proca_keyring_init(void)
{
	const struct cred *cred = current_cred();
	int err = 0;

	proca_keyring = proca_keyring_alloc(proca_keyring_name, KUIDT_INIT(0),
		KGIDT_INIT(0), cred,
		((KEY_POS_ALL & ~KEY_POS_SETATTR) |
		KEY_USR_VIEW | KEY_USR_READ |
		KEY_USR_SEARCH),
		KEY_ALLOC_NOT_IN_QUOTA);
	if (IS_ERR(proca_keyring)) {
		err = PTR_ERR(proca_keyring);
		PROCA_ERROR_LOG("Can't allocate %s keyring (%d)\n",
		proca_keyring_name, err);
		proca_keyring = NULL;
	}

	return err;
}
