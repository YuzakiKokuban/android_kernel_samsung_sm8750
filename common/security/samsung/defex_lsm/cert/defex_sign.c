// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/cred.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/key.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/version.h>
#include <crypto/hash.h>
#include <keys/asymmetric-type.h>
#include "include/defex_config.h"
#include "include/defex_debug.h"
#include "include/defex_sign.h"

#ifdef DEFEX_KUNIT_ENABLED
#include <kunit/mock.h>
#endif

#define SHA256_DIGEST_SIZE	32

#if KERNEL_VER_LESS(3, 7, 0)

__visible_for_testing int defex_public_key_verify_signature(unsigned char *pub_key,
					int pub_key_size,
					unsigned char *signature,
					unsigned char *hash_sha256)
{
	(void)pub_key;
	(void)pub_key_size;
	(void)signature;
	(void)hash_sha256;
	/* Skip signarue check at kernel version < 3.7.0 */
	defex_log_warn("Skip signature check in current kernel version");
	return 0;
}

#else

#include <crypto/public_key.h>

static struct key *defex_keyring;

__visible_for_testing struct key *defex_keyring_alloc(const char *description,
					      kuid_t uid, kgid_t gid,
					      const struct cred *cred,
					      unsigned long flags)
{
#if KERNEL_VER_LESS(3, 8, 0)
	return keyring_alloc(description, uid, gid, cred, flags, NULL)
#elif KERNEL_VER_LESS(4, 7, 0)
	key_perm_t perm = ((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ
		| KEY_USR_SEARCH);

	return keyring_alloc(description, uid, gid, cred, perm, flags, NULL);
#else
	key_perm_t perm = ((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ
		| KEY_USR_SEARCH);

	return keyring_alloc(description, uid, gid, cred, perm, flags, NULL, NULL);
#endif
}

__visible_for_testing int defex_keyring_init(void)
{
	int err = 0;
	const struct cred *cred = current_cred();
	static const char keyring_name[] = "defex_keyring";

	if (defex_keyring)
		return err;

	defex_keyring = defex_keyring_alloc(keyring_name, KUIDT_INIT(0), KGIDT_INIT(0),
					    cred, KEY_ALLOC_NOT_IN_QUOTA);
	if (!defex_keyring) {
		err = -1;
		defex_log_info("Can't allocate %s keyring (NULL)", keyring_name);
	} else if (IS_ERR(defex_keyring)) {
		err = PTR_ERR(defex_keyring);
		defex_log_info("Can't allocate %s keyring, err=%d", keyring_name, err);
		defex_keyring = NULL;
	}
	return err;
}

__visible_for_testing key_ref_t defex_pubkey_prepare(unsigned char *pub_key_start,
		unsigned char *pub_key_end, const char *key_name)
{
	unsigned int pub_key_size = (unsigned int)((pub_key_end - pub_key_start) & 0xffff);
	key_ref_t key_ref = NULL;
	unsigned char *pub_key = NULL;

	if (!pub_key_size || defex_keyring_init() != 0)
		return key_ref;

#if KERNEL_VER_LESS(5, 3, 0)
	key_ref = keyring_search(make_key_ref(defex_keyring, 1), &key_type_asymmetric,
		key_name);
#else
	key_ref = keyring_search(make_key_ref(defex_keyring, 1), &key_type_asymmetric,
		key_name, true);
#endif
	if (IS_ERR_OR_NULL(key_ref)) {
		pub_key = kmalloc(pub_key_size, GFP_KERNEL);
		if (!pub_key)
			goto clean_key_prepare;
		memcpy(pub_key, pub_key_start, pub_key_size);
		key_ref = key_create_or_update(make_key_ref(defex_keyring, 1),
			"asymmetric",
			key_name,
			pub_key,
			pub_key_size,
			((KEY_POS_ALL & ~KEY_POS_SETATTR) | KEY_USR_VIEW | KEY_USR_READ),
			KEY_ALLOC_NOT_IN_QUOTA | KEY_ALLOC_BYPASS_RESTRICTION);
	}
clean_key_prepare:
	if (IS_ERR_OR_NULL(key_ref))
		defex_log_err("Invalid key reference (%ld)", PTR_ERR(key_ref));
	else {
#ifdef DEFEX_DEBUG_ENABLE
		defex_log_info("Key name = %s, Key size = %d", key_name, pub_key_size);
		blob("Key dump:", pub_key_start, pub_key_size, 16);
#endif
	}
	kfree(pub_key);
	return key_ref;
}

__visible_for_testing int defex_public_key_verify_signature(key_ref_t key_ref,
					unsigned char *signature,
					unsigned char *hash_sha256)
{
	int ret = -1;
	struct key *key;
	struct public_key_signature pks;

	if (IS_ERR_OR_NULL(key_ref))
		return ret;

	key = key_ref_to_ptr(key_ref);

	memset(&pks, 0, sizeof(pks));
	pks.digest = hash_sha256;
	pks.digest_size = SHA256_DIGEST_SIZE;
#if KERNEL_VER_LESS(4, 6, 0)

#if KERNEL_VER_GTE(3, 13, 0)
	pks.pkey_algo = PKEY_ALGO_RSA;
#endif
	pks.pkey_hash_algo = HASH_ALGO_SHA256;
	pks.nr_mpi = 1;
	pks.rsa.s = mpi_read_raw_data(signature, SIGN_SIZE);
	if (pks.rsa.s)
		ret = verify_signature(key, &pks);
	mpi_free(pks.rsa.s);
#else
	pks.pkey_algo = "rsa";
	pks.hash_algo = "sha256";
#if KERNEL_VER_GTE(4, 20, 0)
	pks.encoding = "pkcs1";
#endif
	pks.s = signature;
	pks.s_size = SIGN_SIZE;
	ret = verify_signature(key, &pks);
#endif
	key_ref_put(key_ref);
	return ret;
}
#endif /* KERNEL_VER_GTE(3, 7, 0) */

int defex_calc_hash(const char *data, unsigned int size, unsigned char *hash)
{
	struct crypto_shash *handle;
	struct shash_desc *shash;
	int err = -1;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR_OR_NULL(handle)) {
		defex_log_err("Can't alloc sha256");
		return err;
	}

	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
	if (!shash)
		goto clean_handle;

	shash->tfm = handle;

	do {
		err = crypto_shash_init(shash);
		if (err < 0)
			break;

		err = crypto_shash_update(shash, data, size);
		if (err < 0)
			break;

		err = crypto_shash_final(shash, hash);
		if (err < 0)
			break;
	} while (0);

	kfree(shash);
clean_handle:
	crypto_free_shash(handle);
	return err;
}

int defex_rules_signature_check(const char *rules_buffer, unsigned int rules_data_size,
		unsigned int *rules_size)
{
	int res = -1;
	unsigned char *hash_sha256;
	unsigned char *hash_sha256_first;
	unsigned char *signature;
	key_ref_t key_ref;
	static const char key_name_eng[] = "defex_key_eng";
	static const char key_name_usr[] = "defex_key_usr";

	if (rules_data_size < SIGN_SIZE)
		return res;
	hash_sha256_first = kmalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	if (!hash_sha256_first)
		return res;
	hash_sha256 = kmalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	if (!hash_sha256)
		goto clean_hash_sha256_first;
	signature = kmalloc(SIGN_SIZE, GFP_KERNEL);
	if (!signature)
		goto clean_hash_sha256;

	memcpy(signature, (u8 *)(rules_buffer + rules_data_size - SIGN_SIZE), SIGN_SIZE);
	defex_calc_hash(rules_buffer, rules_data_size - SIGN_SIZE, hash_sha256_first);
	defex_calc_hash(hash_sha256_first, SHA256_DIGEST_SIZE, hash_sha256);

#ifdef DEFEX_DEBUG_ENABLE
	defex_log_info("Rules signature size = %d", SIGN_SIZE);
	blob("Rules signature dump:", signature, SIGN_SIZE, 16);
	blob("Final hash dump:", hash_sha256, SHA256_DIGEST_SIZE, 16);
#endif

	/* prepare ENG key */
	key_ref = defex_pubkey_prepare(defex_public_key_eng_start,
		defex_public_key_eng_end, key_name_eng);
	if (!IS_ERR_OR_NULL(key_ref))
		res = defex_public_key_verify_signature(key_ref, signature, hash_sha256);

	if (res != 0) {
		/* prepare USER key */
		key_ref = defex_pubkey_prepare(defex_public_key_usr_start,
			defex_public_key_usr_end, key_name_usr);
		if (!IS_ERR_OR_NULL(key_ref))
			res = defex_public_key_verify_signature(key_ref, signature, hash_sha256);
	}
	if (rules_size && !res)
		*rules_size = rules_data_size - SIGN_SIZE;

	kfree(signature);
clean_hash_sha256:
	kfree(hash_sha256);
clean_hash_sha256_first:
	kfree(hash_sha256_first);
	return res;
}
