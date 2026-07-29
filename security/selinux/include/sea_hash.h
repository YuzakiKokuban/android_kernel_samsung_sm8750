/* SPDX-License-Identifier: GPL-2.0 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <linux/stddef.h>

#define MAX_KEY 64
#define MAX_TABLE 4096

struct HashTable{
	char key[MAX_KEY + 1];
};

unsigned long hash_value(const char *str);
int sea_search(const char *key);
int add(const char *key);
int sea_hash_init(void);
void sea_hash_exit(void);

#endif // HASH_TABLE_H
