// SPDX-License-Identifier: GPL-2.0
/*
 * Character device operation for control PROCA database
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

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
#include <linux/base64.h>
#endif

#include "proca_certificate_db.h"
#include "proca_vfs.h"
#include "proca_log.h"
#include "proca_porting.h"

#define PROCA_CERT_DEV_NAME "proca_cert"
#define MAX_DEV_CMD_SIZE 9UL

extern struct certificates_db proca_test_db;
static dev_t proca_cert_dev;
static struct cdev proca_cert_cdev;
static struct class *proca_cert_class;

enum {
	GET_CMD,
	TEST_LIST_CMD,
	UPDATE,
	MAX_CMD
};

static const char *cmd[] = {
	[GET_CMD] = "GET",
	[TEST_LIST_CMD] = "TEST_LIST",
	[UPDATE] = "update"
};

static void init_db(struct certificates_db *proca_db)
{
	if (atomic_read(&proca_db->status) != INITED &&
		proca_path_is_mounted(proca_db->partition))
		load_db(proca_db->path, proca_db);
}

static ssize_t read_certificate(char **return_buffer, size_t *len,
								char __user *buf, size_t count)
{
	size_t data_len = 0;
	int cert_size = 0;
	char *cert_buff = NULL;
	char *path_buf = NULL;
	char *user_buf = NULL;
	unsigned long cmd_size = strlen(cmd[GET_CMD]);

	/* Parse and load test database in memory if not inited */
	init_db(&proca_test_db);
	/* alloc memory for input pathname */
	user_buf = kzalloc(min(PATH_MAX + cmd_size, count), GFP_KERNEL);
	if (!user_buf)
		return -ENOMEM;

	/* Copy file pathname with command */
	if (copy_from_user(user_buf, buf, min(PATH_MAX + cmd_size, count)) != 0) {
		kfree(user_buf);
		return -EFAULT;
	}

	path_buf = user_buf + cmd_size; /* Skip first 3 bytes of command and get PATH */
	cert_size = __proca_get_certificate_db(path_buf, &cert_buff);
	if (!cert_buff) {
		PROCA_INFO_LOG("Certificate dev: fail to get ceritificate for %s\n", path_buf);
		kfree(user_buf);
		return -EINVAL;
	}

	data_len = cert_size;
	*return_buffer = kzalloc(data_len, GFP_KERNEL);
	if (!(*return_buffer)) {
		kfree(user_buf);
		kfree(cert_buff);
		return -ENOMEM;
	}
	memcpy(*return_buffer, cert_buff, cert_size);
	*len = data_len;

	kfree(user_buf);
	kfree(cert_buff);
	return 0;
}

ssize_t read_all_certificates(char **return_buffer, size_t *len, struct certificates_db *proca_db)
{
	struct certificate_entry *entry = NULL;
	struct list_head *l = NULL;
	size_t data_len = 0;
	size_t str_len = 0;
	char *encoded_cert = NULL;
	char *tmp_buf = NULL;

	/* Parse and load test database in memory if not inited */
	init_db(proca_db);
	if (atomic_read(&proca_db->status) != INITED)
		return -EINVAL;
	mutex_lock(&proca_db->lock);

	/*
	 * Calculate final len of returned data.
	 * Final buffer consists all entry in db.
	 * Buffer with 2 entries is in format:
	 * "<file_name> <encoded_certificate>\n<file_name> <encoded_certificate>\n\0"
	 */
	list_for_each(l, &proca_db->proca_certificates_db.entries) {
		entry = list_entry(l, struct certificate_entry, list);
		if (entry->certificate)
			data_len += entry->file_name_size +
						BASE64_CHARS(entry->certificate_size) + 2;
		else
			data_len += entry->file_name_size + strlen("NULL") + 2;
	}
	data_len += 1; /* add '\0' at the end */
	PROCA_DEBUG_LOG("Certificate dev: size of data = %lu\n", data_len);

	*return_buffer = kzalloc(data_len, GFP_KERNEL);
	if (!(*return_buffer)) {
		mutex_unlock(&proca_db->lock);
		return -ENOMEM;
	}

	/* Fill the final buffer with data */
	tmp_buf = *return_buffer;
	list_for_each(l, &proca_db->proca_certificates_db.entries) {
		entry = list_entry(l, struct certificate_entry, list);
		if (entry->certificate) {
		/* Alloc memory and encode certificate */
			encoded_cert = kzalloc(BASE64_CHARS(entry->certificate_size) + 1,
										GFP_KERNEL);
			if (!encoded_cert) {
				mutex_unlock(&proca_db->lock);
				return -ENOMEM;
			}
			base64_encode((const u8 *)entry->certificate,
				      entry->certificate_size, encoded_cert);
		} else {
			encoded_cert = kzalloc(strlen("NULL") + 1, GFP_KERNEL);
			if (!encoded_cert) {
				mutex_unlock(&proca_db->lock);
				return -ENOMEM;
			}
			memcpy(encoded_cert, "NULL", strlen("NULL"));
		}

		/* Fill the buffer with each entry */
		str_len = entry->file_name_size + strlen(encoded_cert);
		snprintf(tmp_buf, str_len + 3, "%s %s\n", entry->file_name, encoded_cert);
		tmp_buf += str_len + 2;
		kfree(encoded_cert);
	}
	mutex_unlock(&proca_db->lock);
	*len = data_len;

	return 0;
}

/*
 * proca_dev_read_cert() - device read func: read test database
 * There two options how to read db:
 * 1) Get certificate for specific path.
 * 2) Get all certificates.
 *
 * @buf: input buffer to write the certificate(s). In case of readind
 * specific certificate, input buffer must contain command and path:
 * "GET<file_path>".
 *
 * And input command to get all cetrificates form test db:
 * "TEST_LIST".
 */
static ssize_t proca_dev_read_cert(struct file *filp, char __user *buf,
			size_t count, loff_t *f_pos)
{
	ssize_t res = 0;
	char *cmd_buff = NULL;
	char *return_buffer = NULL;
	size_t len = 0;

	/* Need to alloc memory to copy input cmd */
	cmd_buff = kzalloc(min(MAX_DEV_CMD_SIZE + 1, count), GFP_KERNEL);
	if (!cmd_buff)
		return -ENOMEM;

	if (copy_from_user(cmd_buff, buf, min(MAX_DEV_CMD_SIZE, count)) != 0) {
		kfree(cmd_buff);
		return -EFAULT;
	}

	if (strncmp(cmd_buff, cmd[GET_CMD], strlen(cmd[GET_CMD])) == 0) {
		/* Get certificate for the passed file path */
		res = read_certificate(&return_buffer, &len, buf, count);
	} else if (strncmp(cmd_buff, cmd[TEST_LIST_CMD], strlen(cmd[TEST_LIST_CMD])) == 0) {
		/* Read All certificates from test db*/
		res = read_all_certificates(&return_buffer, &len, &proca_test_db);
	}

	if (len) {
		res = simple_read_from_buffer(buf, count, f_pos, return_buffer, len);
		PROCA_DEBUG_LOG("Certificate dev: return buf size = %lu\n", len);
	}

	kfree(cmd_buff);
	kfree(return_buffer);
	return res;
}

/*
 * proca_dev_write_cert() - device write func: update test database
 * In case if database file is changed proca_dev_write_cert() can update
 * database in memory.
 *
 * @buf: input buffer to read the command. To update database, buffer
 * must contain command "update".
 * After getting command, firstly old database is deinited.
 * Then new database file is read, parsed and loaded in memory.
 *
 */
static ssize_t proca_dev_write_cert(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	char *cmd_buf = NULL;
	ssize_t ret = 0;

	if (count >= 0) {
		cmd_buf = kzalloc(count, GFP_KERNEL);
		if (!cmd_buf)
			return -ENOMEM;
	} else
		return -EINVAL;

	ret = simple_write_to_buffer(cmd_buf, count, f_pos, buf, count);
	if (ret <= 0) {
		kfree(cmd_buf);
		return ret;
	}

	if (strncmp(cmd_buf, cmd[UPDATE], strlen(cmd[UPDATE])) == 0) {
		if (atomic_read(&proca_test_db.status) == INITED)
			deinit_proca_db(&proca_test_db);
		if (proca_path_is_mounted(proca_test_db.partition))
			load_db(proca_test_db.path, &proca_test_db);
	}

	kfree(cmd_buf);
	return ret;
}

static const struct file_operations proca_cert_cdev_fops = {
	.owner = THIS_MODULE,
	.read = proca_dev_read_cert,
	.write = proca_dev_write_cert,
};

int __init init_proca_cert_device(void)
{
	if ((alloc_chrdev_region(&proca_cert_dev, 0, 1, PROCA_CERT_DEV_NAME)) < 0) {
		PROCA_ERROR_LOG("Cannot allocate major number\n");
		return -1;
	}

	proca_cert_class = class_create(THIS_MODULE, PROCA_CERT_DEV_NAME);
	if (IS_ERR(proca_cert_class)) {
		PROCA_ERROR_LOG("Cannot create class\n");
		goto region_cleanup;
	}

	cdev_init(&proca_cert_cdev, &proca_cert_cdev_fops);
	if ((cdev_add(&proca_cert_cdev, proca_cert_dev, 1)) < 0) {
		PROCA_ERROR_LOG("Cannot add the device to the system\n");
		goto class_cleanup;
	}

	if (!device_create(proca_cert_class, NULL, proca_cert_dev, NULL, PROCA_CERT_DEV_NAME)) {
		PROCA_ERROR_LOG("Cannot create device\n");
		goto device_cleanup;
	}

	PROCA_INFO_LOG("Certificate device is inited.\n");
	return 0;

device_cleanup:
	cdev_del(&proca_cert_cdev);

class_cleanup:
	class_destroy(proca_cert_class);

region_cleanup:
	unregister_chrdev_region(proca_cert_dev, 1);
	return -1;
}
