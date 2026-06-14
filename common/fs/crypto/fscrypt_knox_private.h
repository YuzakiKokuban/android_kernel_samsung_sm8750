/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fscrypto_knox_private.h
 *
 *  Created on: Oct 11, 2018
 *      Author: olic.moon
 */

#ifndef FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_
#define FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_

union fscrypt_context;

#ifdef CONFIG_DDAR
#include <ddar/dd.h>

#define FSCRYPT_KNOX_FLG_DDAR_SHIFT		8
#define FSCRYPT_KNOX_FLG_DDAR_MASK		0x0000FF00
#define FSCRYPT_KNOX_FLG_DDAR_ENABLED \
			(DD_POLICY_ENABLED << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_USER_SPACE_CRYPTO \
			(DD_POLICY_USER_SPACE_CRYPTO << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_KERNEL_CRYPTO \
			(DD_POLICY_KERNEL_CRYPTO << FSCRYPT_KNOX_FLG_DDAR_SHIFT)
#define FSCRYPT_KNOX_FLG_DDAR_GID_RESTRICTION \
			(DD_POLICY_GID_RESTRICTION << FSCRYPT_KNOX_FLG_DDAR_SHIFT)

static inline int fscrypt_dd_flg_enabled(int flags)
{
	return (flags & FSCRYPT_KNOX_FLG_DDAR_ENABLED) ? 1:0;
}

static inline int fscrypt_dd_flg_userspace_crypto(int flags)
{
	return (flags & FSCRYPT_KNOX_FLG_DDAR_USER_SPACE_CRYPTO) ? 1:0;
}

static inline int fscrypt_dd_flg_kernel_crypto(int flags)
{
	return (flags & FSCRYPT_KNOX_FLG_DDAR_KERNEL_CRYPTO) ? 1:0;
}

static inline int fscrypt_dd_flg_gid_restricted(int flags, int gid)
{
	return (flags & FSCRYPT_KNOX_FLG_DDAR_GID_RESTRICTION) ? 1:0;
}

int update_encryption_context_with_dd_policy(
		struct inode *inode,
		const struct dd_policy *policy);

void *dd_get_info(const struct inode *inode);

void fscrypt_dd_set_count(long count);
long fscrypt_dd_get_count(void);
void fscrypt_dd_inc_count(void);
void fscrypt_dd_dec_count(void);
int fscrypt_dd_is_locked(void);
#endif

#endif /* FS_CRYPTO_FSCRYPT_KNOX_PRIVATE_H_ */
