// SPDX-License-Identifier: GPL-2.0-only

#include <linux/string.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/init.h>
#include "sea_hash.h"

static struct HashTable sea_hash_table[MAX_TABLE];

unsigned long hash_value(const char *str)
{
	unsigned long hash = 5381;
	int c = *str;

	while (c != 0) {
		hash = (((hash << 5) + hash) + c) %
		       MAX_TABLE; /* hash * 33 + c */
		c = *str++;
	}
	return hash % MAX_TABLE;
}

int sea_search(const char *key)
{
	unsigned long h = hash_value(key);
	int cnt = MAX_TABLE;

	while (sea_hash_table[h].key[0] != 0 && cnt--) {
		if (strcmp(sea_hash_table[h].key, key) == 0) {
			pr_info("%s: Key match found\n", __func__);
			return 1;
		}
		h = (h + 1) % MAX_TABLE;
	}
	return 0;
}

int add(const char *key)
{
	unsigned long h = hash_value(key);

	while (sea_hash_table[h].key[0] != 0) {
		if (strcmp(sea_hash_table[h].key, key) == 0)
			return 0;
		h = (h + 1) % MAX_TABLE;
	}
	strscpy(sea_hash_table[h].key, key, sizeof(sea_hash_table[h].key));
	return 1;
}

int __init sea_hash_init(void)
{
	add("u:object_r:efsblk_device:s0");
	add("u:object_r:sec_efsblk_device:s0");
	add("u:object_r:steady_block_device:s0");
	add("u:object_r:super_block_device:s0");
	add("u:object_r:nfc_data_file:s0");
	add("u:object_r:bluetooth_data_file:s0");
	add("u:object_r:face_vendor_data_file:s0");
	add("u:object_r:knox_dar_data_file:s0");
	add("u:object_r:ker_data_file:s0");
	add("u:object_r:backup_data_file:s0");
	add("u:object_r:staging_data_file:s0");
	add("u:object_r:vendor_samsungpass_data_file:s0");
	add("u:object_r:vold_data_file:s0");
	add("u:object_r:fota_data_file:s0");
	return 1;
}

void __exit sea_hash_exit(void)
{
	//Module Exit
}

module_init(sea_hash_init);
module_exit(sea_hash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Avijit Borah <avijit.borah@samsung.com>");
MODULE_AUTHOR("Ankit Kataria <a.kataria@samsung.com>");
MODULE_AUTHOR("Suraj Kishor Thakre <suraj.t@samsung.com>");
