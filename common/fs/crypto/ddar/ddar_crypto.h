/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DDAR_CRYPTO_H_
#define DDAR_CRYPTO_H_

#include <linux/crypto.h>
#include <linux/init.h>
#include "../fscrypt_private.h"
#include "../fscrypt_knox_private.h"

#define DDAR_CRYPTO_RNG_SEED_SIZE 32

struct ext_fscrypt_inode_info {
	struct fscrypt_inode_info fscrypt_info;
	struct dd_info *ci_dd_info;
};

static inline struct ext_fscrypt_inode_info *GET_EXT_CI(struct fscrypt_inode_info *ci)
{
	return container_of(ci, struct ext_fscrypt_inode_info, fscrypt_info);
}

static inline bool fscrypt_ddar_protected(const u32 knox_flags)
{
	if (knox_flags & FSCRYPT_KNOX_FLG_DDAR_ENABLED)
		return true;

	return false;
}

static inline int fscrypt_set_knox_ddar_flags(union fscrypt_context *ctx_u,
						struct fscrypt_inode_info *crypt_info)
{
	struct ext_fscrypt_inode_info *ext_crypt_info;

	if (!crypt_info)
		return 0;

	ext_crypt_info = GET_EXT_CI(crypt_info);
	if (!ext_crypt_info->ci_dd_info)
		return 0;

	switch (ctx_u->version) {
	case FSCRYPT_CONTEXT_V1: {
		struct fscrypt_context_v1 *ctx = &ctx_u->v1;

		ctx->knox_flags |= ((ext_crypt_info->ci_dd_info->policy.flags
				<< FSCRYPT_KNOX_FLG_DDAR_SHIFT) & FSCRYPT_KNOX_FLG_DDAR_MASK);
		return 0;
	}
	case FSCRYPT_CONTEXT_V2: {
		struct fscrypt_context_v2 *ctx = &ctx_u->v2;

		ctx->knox_flags |= ((ext_crypt_info->ci_dd_info->policy.flags
				<< FSCRYPT_KNOX_FLG_DDAR_SHIFT) & FSCRYPT_KNOX_FLG_DDAR_MASK);
		return 0;
	}
	}
	/* unreachable */
	return -EINVAL;
}

static inline struct fscrypt_inode_info *fscrypt_has_dar_info(struct inode *parent)
{
	struct fscrypt_inode_info *ci = fscrypt_get_inode_info(parent);
	struct ext_fscrypt_inode_info *ext_ci;

	if (ci) {
		ext_ci = GET_EXT_CI(ci);
		if (ext_ci->ci_dd_info)
			return ci;
	}
	return NULL;
}

static inline bool fscrypt_has_knox_flags(const union fscrypt_context *ctx_u)
{
	switch (ctx_u->version) {
	case FSCRYPT_CONTEXT_V1: {
		const struct fscrypt_context_v1 *ctx = &ctx_u->v1;

		return (ctx->knox_flags != 0) ? true : false;
	}
	case FSCRYPT_CONTEXT_V2: {
		const struct fscrypt_context_v2 *ctx = &ctx_u->v2;

		return (ctx->knox_flags != 0) ? true : false;
	}
	}
	return false;
}

static inline u32 fscrypt_knox_flags_from_context(const union fscrypt_context *ctx_u)
{
	switch (ctx_u->version) {
	case FSCRYPT_CONTEXT_V1: {
		const struct fscrypt_context_v1 *ctx = &ctx_u->v1;

		return ctx->knox_flags;
	}
	case FSCRYPT_CONTEXT_V2: {
		const struct fscrypt_context_v2 *ctx = &ctx_u->v2;

		return ctx->knox_flags;
	}
	}
	return 0;
}

extern int fscrypt_dd_decrypt_page(struct inode *inode, struct page *page);
extern int fscrypt_dd_encrypted(struct bio_vec *bv);
extern int fscrypt_dd_encrypted_inode(const struct inode *inode);
extern int fscrypt_dd_is_traced_inode(const struct inode *inode);
extern void fscrypt_dd_trace_inode(const struct inode *inode);
extern long fscrypt_dd_get_ino(struct bio *bio);
extern long fscrypt_dd_ioctl(unsigned int cmd, unsigned long *arg, struct inode *inode);
extern int fscrypt_dd_submit_bio(struct inode *inode, struct bio *bio);
extern int fscrypt_dd_may_submit_bio(struct bio *bio);
extern struct inode *fscrypt_bio_get_inode(const struct bio_vec *bv);
extern bool fscrypt_dd_can_merge_bio(struct bio *bio, struct address_space *mapping);

int fscrypt_get_encryption_key(
		struct fscrypt_inode_info *crypt_info,
		struct fscrypt_key *key);
int fscrypt_get_encryption_kek(
		struct fscrypt_inode_info *crypt_info,
		struct fscrypt_key *kek);

/* Declarations for Open APIs*/
int ddar_crypto_generate_key(void *raw_key, int nbytes);
int __init ddar_crypto_init(void);
void __exit ddar_crypto_exit(void);

#endif /* DDAR_CRYPTO_H_ */
