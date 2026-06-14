// SPDX-License-Identifier: GPL-2.0
/*
 * DB with PROCA certificates
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
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

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/version.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/crypto.h>
#include <crypto/hash.h>
#include <crypto/hash_info.h>
#include <linux/namei.h>

#include "proca_certificate_db.h"
#include "proca_log.h"
#include "proca_vfs.h"
#include "proca_porting.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 42)
#include "proca_certificate_db.asn1.h"
#else
#include "proca_certificate_db-asn1.h"
#endif

struct certificates_db proca_test_db = {
	.name = "test", .path = "/data/proca.db", .partition = "data",};
static struct certificates_db system_db = {
	.name = "system", .path = "/system/etc/proca.db", .partition = "system"};
static struct certificates_db vendor_db = {
	.name = "vendor", .path = "/vendor/etc/proca.db", .partition = "vendor"};
static struct list_head proca_dbs;

static struct crypto_shash *g_db_validation_shash;

static int proca_verify_digsig(struct certificates_db *db);
static int proca_calc_data_shash(const u8 *data, size_t data_len,
				u8 *hash, size_t *hash_len);

static inline bool is_test_db(struct certificates_db *db)
{
	if (db == &proca_test_db)
		return true;

	return false;
}

int proca_certificate_db_get_filename(void *context, size_t hdrlen,
				   unsigned char tag,
				   const void *value, size_t vlen)
{
	struct certificate_db *db = context;
	struct certificate_entry *entry;

	if (!db || !value || !vlen)
		return -EINVAL;

	// create new entry
	entry = kzalloc(sizeof(struct certificate_entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;
	list_add(&entry->list, &db->entries);

	entry->file_name = kmalloc(vlen + 1, GFP_KERNEL);
	if (!entry->file_name)
		return -ENOMEM;

	memcpy(entry->file_name, value, vlen);
	entry->file_name[vlen] = '\0';
	entry->file_name_size = vlen;

	PROCA_INFO_LOG("Load certificate for %s.\n", entry->file_name);

	return 0;
}

int proca_certificate_db_get_certificate(void *context, size_t hdrlen,
				   unsigned char tag,
				   const void *value, size_t vlen)
{
	struct certificate_db *db = context;
	struct certificate_entry *entry;

	if (!db)
		return -EINVAL;

	entry = list_first_entry(&db->entries, struct certificate_entry, list);

	if (!value || !vlen) {
		entry->certificate = NULL;
		entry->certificate_size = 0;
		return 0;
	}

	entry->certificate = kmalloc(vlen + 1, GFP_KERNEL);
	if (!entry->certificate)
		return -ENOMEM;

	memcpy(entry->certificate, value, vlen);
	entry->certificate[vlen] = '\0';
	entry->certificate_size = vlen;

	return 0;
}

int proca_certificate_db_get_signed_data(void *context, size_t hdrlen,
						  unsigned char tag,
						  const void *value, size_t vlen)
{
	struct certificate_db *certificate = context;
	struct certificates_db *db = container_of(certificate,
						  struct certificates_db, proca_certificates_db);
	struct signed_db *signed_db = &db->proca_signed_db;
	int rc = 0;
	uint8_t request_hash[PROCA_DB_MAX_DIGEST_SIZE];
	size_t request_hash_size = PROCA_DB_MAX_DIGEST_SIZE;

	if (is_test_db(db)) /* there is no signature in test db */
		return 0;

	if (!certificate || !value || !vlen ||
		!((const u8 *)value - hdrlen)) /* check all data with header */
		return -EINVAL;

	signed_db->db_hash = kmalloc(PROCA_DB_MAX_DIGEST_SIZE, GFP_KERNEL);
	if (!signed_db->db_hash)
		return -ENOMEM;

	signed_db->db_hash_size = PROCA_DB_MAX_DIGEST_SIZE;
	memset(signed_db->db_hash, 0, signed_db->db_hash_size);

	/* During the signing process, both data with a header is hashed.
	 * To calculate and verify hash correctly make offset to data with header.
	 */
	rc = proca_calc_data_shash((const u8 *)value - hdrlen, vlen +  hdrlen,
								request_hash, &request_hash_size);
	if (unlikely(rc)) {
		PROCA_INFO_LOG("Failed to calculate request hash\n");
		return rc;
	}

	rc = proca_calc_data_shash((const u8 *)request_hash, request_hash_size,
				   signed_db->db_hash, &signed_db->db_hash_size);
	if (unlikely(rc)) {
		PROCA_INFO_LOG("Failed to calculate db hash\n");
		return rc;
	}

	return 0;
}

int proca_certificate_db_get_signature(void *context, size_t hdrlen,
						  unsigned char tag,
						  const void *value, size_t vlen)
{
	struct certificate_db *certificate = context;
	struct certificates_db *db = container_of(certificate,
						  struct certificates_db, proca_certificates_db);
	struct signed_db *signed_db = &db->proca_signed_db;

	if (is_test_db(db)) /* there is no signature in test db */
		return 0;

	if (!certificate || !value || !vlen)
		return -EINVAL;

	signed_db->signature = kmalloc(vlen, GFP_KERNEL);
	if (!signed_db->signature)
		return -ENOMEM;
	memcpy(signed_db->signature, value, vlen);
	signed_db->signature_size = vlen;

	return 0;
}

int proca_certificate_db_get_key_id(void *context, size_t hdrlen,
						  unsigned char tag,
						  const void *value, size_t vlen)
{
	char buff[12] = {0};
	struct certificate_db *certificate = context;
	struct certificates_db *db = container_of(certificate,
						  struct certificates_db, proca_certificates_db);

	if (!db || !value || !vlen)
		return -EINVAL;

	memcpy(buff, value, vlen);
	if (kstrtouint(buff, 10, &(db->key_id)) != 0)
		return -EINVAL;

	return 0;
}

int parse_proca_db(const char *certificate_buff,
				const size_t buff_size,
				struct certificate_db *db)
{
	int rc = 0;

	INIT_LIST_HEAD(&db->entries);

	rc = asn1_ber_decoder(&proca_certificate_db_decoder, db,
			      certificate_buff,
			      buff_size);

	return rc;
}

void deinit_proca_db(struct certificates_db *db)
{
	struct list_head *l;
	struct certificate_entry *entry;
	struct certificate_db *cert_db = &db->proca_certificates_db;

	mutex_lock(&db->lock);
	list_for_each(l, &cert_db->entries) {
		entry = list_entry(l, struct certificate_entry, list);
		kfree(entry->file_name);
		kfree(entry->certificate);
	}

	kfree(db->proca_signed_db.db_hash);
	kfree(db->proca_signed_db.signature);
	mutex_unlock(&db->lock);
	atomic_set(&db->status, NOT_READY);
}

struct certificate_entry *proca_certificate_db_find_entry(
	struct certificates_db *db, const char *path)
{
	struct list_head *l;
	struct certificate_entry *entry = NULL;

	/* Check that DB is inited */
	if (atomic_read(&db->status) != INITED)
		return NULL;

	list_for_each(l, &db->proca_certificates_db.entries) {
		entry = list_entry(l, struct certificate_entry, list);
		if (strncmp(path, entry->file_name, entry->file_name_size) == 0)
			return entry;
	}

	return NULL;
}

/*
 * proca_db_is_ready() - Verify if partition of database is
 * mounted and actual db file exist
 */
static bool proca_db_is_ready(const char *partition, const char *db_path)
{
	struct path path;
	int error;

	/* Checks if the partition where the database file is located exists */
	if (!proca_path_is_mounted(partition))
		return false;

	/* Check if database file exist */
	error = kern_path(db_path, LOOKUP_FOLLOW, &path);
	if (error)
		return false;

	path_put(&path);
	return true;
}

int __proca_get_certificate_db(const char *pathname, char **certificate)
{
	struct certificate_entry *entry = NULL;
	bool check_system, check_vendor;

#if defined(CONFIG_PROCA_DEBUG)

	if (atomic_read(&proca_test_db.status) == NOT_READY &&
		proca_db_is_ready(proca_test_db.partition, proca_test_db.path)) {
		load_db(proca_test_db.path, &proca_test_db);
	}

	mutex_lock(&proca_test_db.lock);
	entry = proca_certificate_db_find_entry(&proca_test_db,
		pathname);

	if (entry) {
		PROCA_INFO_LOG("Certificate for '%s' is found in TEST DB.\n", entry->file_name);

		if (certificate && entry->certificate) {
			*certificate = kmemdup(entry->certificate,
			entry->certificate_size, GFP_KERNEL);
		}

		mutex_unlock(&proca_test_db.lock);
		return entry->certificate_size;
	}
	mutex_unlock(&proca_test_db.lock);

#endif

	check_system = str_has_prefix(pathname, "/system");
	check_vendor = str_has_prefix(pathname, "/vendor");
	if (!check_vendor && !check_system)
		check_system = check_vendor = true;

	if (check_system) {
		if (atomic_read(&system_db.status) == NOT_READY &&
			proca_db_is_ready(system_db.partition, system_db.path)) {
			load_db(system_db.path, &system_db);
		}

		entry = proca_certificate_db_find_entry(&system_db,
			pathname);
		if (entry)
			goto exit;
	}

	if (check_vendor) {
		if (atomic_read(&vendor_db.status) == NOT_READY &&
			proca_db_is_ready(vendor_db.partition, vendor_db.path)) {
			load_db(vendor_db.path, &vendor_db);
		}

		entry = proca_certificate_db_find_entry(&vendor_db,
			pathname);
		if (entry)
			goto exit;
	}

exit:
	if (entry) {
		PROCA_INFO_LOG("Certificate for '%s' is found.\n", entry->file_name);
		if (certificate && entry->certificate) {
			*certificate = kmemdup(entry->certificate,
				entry->certificate_size, GFP_KERNEL);
		}

		return entry->certificate_size;
	}

	return -1;
}

int proca_get_certificate_db(struct file *file, char **certificate)
{
	const char *pathname = NULL;
	char *pathbuf = NULL;
	char filename[NAME_MAX];
	int ret = 0;

	if (!file)
		return -EINVAL;

	if (certificate)
		*certificate = NULL;

	pathname = proca_d_path(file, &pathbuf, filename);
	if (!pathbuf)
		return -ENOMEM;

	ret = __proca_get_certificate_db(pathname, certificate);
	__putname(pathbuf);

	return ret;
}

bool proca_is_certificate_present_db(struct file *file)
{
	return proca_get_certificate_db(file, NULL) >= 0;
}

int load_db(const char *file_path,
		struct certificates_db *proca_db)
{
	struct file *f;
	int data_size, db_size, res = -1;
	unsigned char *data_buff = NULL;
	struct certificate_db *db;
	long error_code;

	if (atomic_read(&proca_db->status) == INITED)
		return 0;

	f = proca_kernel_open(file_path, O_RDONLY, 0);
	if (IS_ERR(f)) {
		error_code = PTR_ERR(f);
		if (error_code == -ENOENT)
			atomic_set(&proca_db->status, ABSENT);

		PROCA_ERROR_LOG("Failed to open DB file '%s' (%ld)\n",
				file_path, (long)PTR_ERR(f));
		goto do_exit;
	}

	data_size = i_size_read(file_inode(f));
	if (data_size <= 0 || data_size > 10 * 1024 * 1024)
		goto do_clean;
	data_buff = vmalloc(data_size);
	if (!data_buff)
		goto do_clean;

	db_size = proca_kernel_read(f, 0, data_buff, data_size);
	if (db_size <= 0) {
		PROCA_ERROR_LOG("Failed to read DB file (%d)\n", db_size);
		goto do_clean;
	}

	PROCA_INFO_LOG("Read %d bytes.\n", db_size);

	mutex_lock(&proca_db->lock);

	if (atomic_read(&proca_db->status) == INITED) {
		mutex_unlock(&proca_db->lock);
		goto do_clean;
	}

	db = &proca_db->proca_certificates_db;
	res = parse_proca_db(data_buff, db_size, db);
	if (res) {
		mutex_unlock(&proca_db->lock);
		PROCA_ERROR_LOG("Failed to parse DB asn1 data\n");
		deinit_proca_db(proca_db);
		goto do_clean;
	}

	if (is_test_db(proca_db)) /* don't need verify signature for test db */
		atomic_set(&proca_db->status, INITED);
	else {
		res = proca_verify_digsig(proca_db);
		if (res) {
			mutex_unlock(&proca_db->lock);
			PROCA_ERROR_LOG("Failed to verify DB digsig\n");
			deinit_proca_db(proca_db);
			goto do_clean;
		}

		kfree(proca_db->proca_signed_db.db_hash);
		proca_db->proca_signed_db.db_hash = NULL;
		kfree(proca_db->proca_signed_db.signature);
		proca_db->proca_signed_db.signature = NULL;
		atomic_set(&proca_db->status, INITED);
	}
	mutex_unlock(&proca_db->lock);

do_clean:
	filp_close(f, NULL);
	if (data_buff)
		vfree(data_buff);
do_exit:
	return res;
}

static int init_db_validation_hash(void)
{
	g_db_validation_shash = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(g_db_validation_shash)) {
		PROCA_ERROR_LOG("can't alloc sha256 alg, rc - %ld.\n",
			PTR_ERR(g_db_validation_shash));
		return PTR_ERR(g_db_validation_shash);
	}
	return 0;
}

static int proca_calc_hash_tfm(const u8 *data, size_t data_len,
		u8 *hash, size_t *hash_len, struct crypto_shash *tfm)
{
	SHASH_DESC_ON_STACK(shash, tfm);
	const size_t len = crypto_shash_digestsize(tfm);
	int rc;

	if (*hash_len < len || data_len == 0)
		return -EINVAL;

	shash->tfm = tfm;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
	shash->flags = 0;
#endif

	rc = crypto_shash_init(shash);
	if (rc != 0)
		return rc;

	rc = crypto_shash_update(shash, data, data_len);
	if (!rc) {
		rc = crypto_shash_final(shash, hash);
		if (!rc)
			*hash_len = len;
	}

	return rc;
}

static int proca_calc_data_shash(const u8 *data, size_t data_len,
				u8 *hash, size_t *hash_len)
{
	struct crypto_shash *tfm;
	int rc;

	tfm = g_db_validation_shash;
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	rc = proca_calc_hash_tfm(data, data_len, hash, hash_len, tfm);

	return rc;
}

static int proca_verify_digsig(struct certificates_db *db)
{
	int rc = 0;
	struct signed_db *sig_db = &db->proca_signed_db;

	rc = proca_digsig_verify(sig_db->signature, sig_db->signature_size,
				 sig_db->db_hash, sig_db->db_hash_size, db->key_id);

	return rc;
}

int __init proca_certificate_db_init(void)
{
	struct list_head *l;
	struct certificates_db *db;

	INIT_LIST_HEAD(&proca_dbs);
	list_add(&proca_test_db.list, &proca_dbs);
	list_add(&system_db.list, &proca_dbs);
	list_add(&vendor_db.list, &proca_dbs);

	list_for_each(l, &proca_dbs) {
		db = list_entry(l, struct certificates_db, list);
		mutex_init(&db->lock);
		atomic_set(&db->status, NOT_READY);
	}

	init_db_validation_hash();
	return 0;
}

void __exit proca_certificate_db_deinit(void)
{
	struct list_head *l;
	struct certificates_db *db;

	list_for_each(l, &proca_dbs) {
		db = list_entry(l, struct certificates_db, list);
		if (atomic_read(&db->status) == INITED)
			deinit_proca_db(db);
		atomic_set(&db->status, ABSENT);
	}
	crypto_free_shash(g_db_validation_shash);
}
