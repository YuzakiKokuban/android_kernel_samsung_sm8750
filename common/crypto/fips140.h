/* SPDX-License-Identifier: GPL-2.0 */
//
// In Samsung R&D Institute Ukraine, LLC (SRUKR) under a contract between
// Samsung R&D Institute Ukraine, LLC (Kyiv, Ukraine)
// and "Samsung Electronics Co", Ltd (Seoul, Republic of Korea)
// Copyright: (c) Samsung Electronics Co, Ltd 2024. All rights reserved.
//

#ifndef _CRYPTO_FIPS140_H
#define _CRYPTO_FIPS140_H

#include <linux/kernel.h>
#include <linux/module.h>

#ifdef CONFIG_CRYPTO_SKC_FIPS_FUNC_TEST
#include "fips140_test.h"
#endif

#if !(IS_BUILTIN(CONFIG_CRYPTO_HMAC) && \
	IS_BUILTIN(CONFIG_CRYPTO_SHA512) && \
	IS_BUILTIN(CONFIG_CRYPTO_SHA256) && \
	IS_BUILTIN(CONFIG_CRYPTO_SHA1) && \
	IS_BUILTIN(CONFIG_CRYPTO_ECB) && \
	IS_BUILTIN(CONFIG_CRYPTO_CBC) && \
	IS_BUILTIN(CONFIG_CRYPTO_DRBG_HMAC) && \
	IS_BUILTIN(CONFIG_CRYPTO_SHA1_ARM64_CE) && \
	IS_BUILTIN(CONFIG_CRYPTO_SHA2_ARM64_CE) && \
	IS_BUILTIN(CONFIG_CRYPTO_AES_ARM64_CE_BLK))
	#error "FIPS module lacks crypto. Please, check the relevant crypto configs are enabled."
#endif

#define SKC_VERSION_TEXT "Samsung Kernel Cryptographic Module v2.5"
#define FIPS140_ERR 1
#define FIPS140_NO_ERR 0

#define FIPS_HMAC_SIZE         (32)
#define FIPS_CRYPTO_ADDRS_SIZE (4096)

struct first_last {
	aligned_u64 first;
	aligned_u64 last;
};

extern const volatile uint64_t crypto_buildtime_address;
extern const volatile struct first_last integrity_crypto_addrs[FIPS_CRYPTO_ADDRS_SIZE];
extern const volatile uint8_t buildtime_crypto_hmac[FIPS_HMAC_SIZE];

extern int do_integrity_check(void);

uint32_t skc_is_approved_service(const char *alg_name);
const char *skc_module_get_version(void);

#ifdef CONFIG_CRYPTO_SKC_FIPS_FUNC_TEST
void reset_in_fips_err(void);
#endif /* CONFIG_CRYPTO_SKC_FIPS_FUNC_TEST */

#endif /* _CRYPTO_FIPS140_H */
