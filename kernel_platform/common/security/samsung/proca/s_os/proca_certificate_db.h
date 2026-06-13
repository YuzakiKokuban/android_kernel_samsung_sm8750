/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef _LINUX_PROCA_CERTIFICATE_DB_H
#define _LINUX_PROCA_CERTIFICATE_DB_H

#include <linux/file.h>
#include <linux/types.h>

#include "proca_porting.h"

#define PROCA_DB_MAX_DIGEST_SIZE 64

struct certificate_entry {
	char *file_name;
	size_t file_name_size;

	char *certificate;
	size_t certificate_size;
	struct list_head list;
};

struct certificate_db {
	struct list_head entries;
};

struct signed_db {
	char *db_hash;
	size_t db_hash_size;
	char *signature;
	size_t signature_size;
};

enum db_status {
	NOT_READY,
	INITED,
	ABSENT,
};

struct certificates_db {
	struct certificate_db proca_certificates_db;
	struct signed_db proca_signed_db;
	struct mutex lock;
	atomic_t status;
	const char *name;
	const char *path;
	const char *partition;
	uint32_t key_id;
	struct list_head list;
};

int parse_proca_db(const char *certificate_buff,
		const size_t buff_size,
		struct certificate_db *db);

void deinit_proca_db(struct certificates_db *db);

int load_db(const char *file_path,
		struct certificates_db *proca_db);

int proca_digsig_verify(const char *signature, int sig_len,
			const char *hash, int hash_len, uint32_t key_id);

int __init proca_certificate_db_init(void);
void __exit proca_certificate_db_deinit(void);

int __init proca_keyring_init(void);
int __init proca_load_built_x509(void);

#ifdef CONFIG_PROCA_CERT_DEVICE
int __init init_proca_cert_device(void);
#endif

/*
 * Public API for certificate DB
 * This is the main functionality of the PROCA database,
 * for reading, searching and verifying file ceritificate.
 */

/*
 * proca_get_certificate_db() - Read certificate for specific file.
 * @file: The file struct to get certificate for.
 * @certificate: Buffer to copy certificate
 *
 * Return: Size of certificate on success or error on failure.
 */
int proca_get_certificate_db(struct file *file, char **certificate);

/*
 * __proca_get_certificate_db() - Read certificate for specific file path.
 * @pathname: The file path to get certificate for.
 * @certificate: Buffer to copy certificate.
 *
 * Return: Size of certificate on success or error on failure.
 */
int __proca_get_certificate_db(const char *pathname, char **certificate);

/*
 * proca_is_certificate_present_db() - Check if file certificate exist.
 * @file: The file struct to check certificate for.
 *
 * Return: True if certificate presents or false if certificate for file not found.
 */
bool proca_is_certificate_present_db(struct file *file);

/*
 * proca_certificate_db_find_entry() - Searches the database entry for the passed file path.
 * @path: The file path to get entry for.
 *
 * Return: certificate_entry if certificate presents or NULL if certificate for file not found.
 */
struct certificate_entry *proca_certificate_db_find_entry(struct certificates_db *db,
									const char *path);

#endif /* _LINUX_PROCA_CERTIFICATE_DB_H */
