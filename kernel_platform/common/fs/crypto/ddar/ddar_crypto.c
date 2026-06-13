// SPDX-License-Identifier: GPL-2.0
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

#include <crypto/hash.h>
#include <crypto/rng.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/random.h>
#include <linux/types.h>

#include "ddar_crypto.h"

int __init ddar_crypto_init(void)
{
	// Defer effective initialization until the time /dev/random is ready.
	pr_info("ddar_crypto: initialized!\n");
	return 0;
}

/* Codes are extracted from fs/crypto/crypto_sec.c */
#ifdef CONFIG_CRYPTO_SKC_FIPS
static struct crypto_rng *ddar_crypto_rng;
#endif
static struct crypto_shash *sha512_tfm;

#ifdef CONFIG_CRYPTO_SKC_FIPS
static int ddar_crypto_init_rng(void)
{
	struct crypto_rng *jitter_rng = NULL;
	struct crypto_rng *rng = NULL;
	unsigned char *rng_seed = NULL;
	int res = 0;

	/* already initialized */
	if (ddar_crypto_rng) {
		pr_err("ddar_crypto: ddar_crypto_rng already initialized\n");
		return 0;
	}

	rng_seed = kmalloc(DDAR_CRYPTO_RNG_SEED_SIZE, GFP_KERNEL);
	if (!rng_seed) {
		res = -ENOMEM;
		goto out;
	}
	memset((void *)rng_seed, 0, DDAR_CRYPTO_RNG_SEED_SIZE);

	jitter_rng = crypto_alloc_rng("jitterentropy_rng", 0, 0);
	if (IS_ERR(jitter_rng)) {
		pr_err("ddar_crypto: failed to allocate jitter_rng(%ld)\n",
				PTR_ERR(jitter_rng));
		res = PTR_ERR(jitter_rng);
		jitter_rng = NULL;
		goto out;
	}

	res = crypto_rng_get_bytes(jitter_rng, rng_seed, DDAR_CRYPTO_RNG_SEED_SIZE);
	if (res < 0)
		pr_err("ddar_crypto: get rng seed fail (%d)\n", res);

	// create drbg for random number generation
	rng = crypto_alloc_rng("stdrng", 0, 0);
	if (IS_ERR(rng)) {
		pr_err("ddar_crypto: failed to allocate rng(%ld)\n", PTR_ERR(rng));
		res = PTR_ERR(rng);
		rng = NULL;
		goto out;
	}

	// push seed to drbg
	res = crypto_rng_reset(rng, rng_seed, DDAR_CRYPTO_RNG_SEED_SIZE);
	if (res < 0)
		pr_err("ddar_crypto: rng reset fail (%d)\n", res);
out:
	if (res && rng) {
		crypto_free_rng(rng);
		rng = NULL;
	}
	kfree(rng_seed);
	if (jitter_rng) {
		pr_debug("ddar_crypto: release jitter rng!\n");
		crypto_free_rng(jitter_rng);
		jitter_rng = NULL;
	}

	// save rng on global variable
	ddar_crypto_rng = rng;
	return res;
}
#endif

int ddar_crypto_generate_key(void *raw_key, int nbytes)
{
#ifdef CONFIG_CRYPTO_SKC_FIPS
	int res;
	int trial = 10;

	if (likely(ddar_crypto_rng)) {
again:
		WARN_ON_ONCE(!ddar_crypto_rng);
		return crypto_rng_get_bytes(ddar_crypto_rng,
			raw_key, nbytes);
	}

	do {
		res = ddar_crypto_init_rng();
		if (!res)
			goto again;

		pr_debug("ddar_crypto: try to sdp_crypto_init_rng(%d)\n", trial);
		msleep(500);
		trial--;

	} while (trial > 0);

	pr_err("ddar_crypto: failed to initialize rng(err:%d)\n", res);
	return res;
#else
	get_random_bytes(raw_key, nbytes);
	return 0;
#endif
}

static void ddar_crypto_exit_rng(void)
{
#ifdef CONFIG_CRYPTO_SKC_FIPS
	if (!ddar_crypto_rng)
		return;

	pr_debug("ddar_crypto: release ddar crypto rng!\n");
	crypto_free_rng(ddar_crypto_rng);
	ddar_crypto_rng = NULL;
#else
	return;
#endif
}

static void __exit ddar_crypto_exit_sha512(void)
{
	crypto_free_shash(sha512_tfm);
	sha512_tfm = NULL;
}

void __exit ddar_crypto_exit(void)
{
	ddar_crypto_exit_rng();
	ddar_crypto_exit_sha512();
}
