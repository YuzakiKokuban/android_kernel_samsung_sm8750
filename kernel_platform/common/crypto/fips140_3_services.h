/* SPDX-License-Identifier: GPL-2.0 */
//
// In Samsung R&D Institute Ukraine, LLC (SRUKR) under a contract between
// Samsung R&D Institute Ukraine, LLC (Kyiv, Ukraine)
// and "Samsung Electronics Co", Ltd (Seoul, Republic of Korea)
// Copyright: (c) Samsung Electronics Co, Ltd 2024. All rights reserved.
//

#ifndef _CRYPTO_FIPS140_3_SERVICES_H
#define _CRYPTO_FIPS140_3_SERVICES_H

#include <linux/kernel.h>

/**
 * The API creates and handles the list of property records of the allocated
 * crypto transformations. Each list item must be inited and finalized by the client
 * of the service. Each record contains property(s) which is(are) specific to the
 * crypto transformation.
 *
 * Using example:
 *
 *		const char *crypto_id = "hmac(sha256-generic)";
 *		uint32_t approve_status = 0;
 *
 *		struct crypto_shash *tfm = crypto_alloc_shash(crypto_id, 0, 0);
 *
 *		ret = init_service_indicator(tfm, crypto_id, GFP_KERNEL);
 *		if (ret)
 *			...
 *
 *		...
 *		...
 *		crypto_shash_init(...);
 *		crypto_shash_update(...);
 *		crypto_shash_final(...);
 *
 *		approve_status = get_service_status(tfm);
 *
 *		finit_service_indicator(tfm);
 *		crypto_free_shash(tfm);
 *
 *		switch (approve_status) {
 *		case 0:
 *			pr_info("The service is NON-approved.\n)";
 *			break;
 *		case 1:
 *			pr_info("The service is approved.\n");
 *			break;
 *		default:
 *			pr_info("The service status evaluation is failed.\n");
 *		}
 */

/**
 * Allocate and deploy a new item of the service properties.
 * If the function is called for the existing item of the properties list,
 * the property record gets cleaned.
 *
 * @tfm: pointer to crypto transformation handler.
 * @crypto_id: pointer to crypto transformation id.
 * @flags: flags for memory allocation to be used.
 * Return: 0 in case of success or non-zero otherwise.
 */
int init_service_indicator(const void *tfm, const char *crypto_id, gfp_t flags);

/**
 * Release an item of the service properties.
 * The API releases resources allocated at |init_service_indicator| call.
 *
 * @tfm: crypto transformation handler.
 */
void finit_service_indicator(const void *tfm);

/**
 * Return final status of the service properties.
 *
 * @tfm: crypto transformation handler.
 * Return:
 * 1 if the service is in approved state,
 * 0 if the service is in non-approved state,
 * -EINVAL if internal error happened.
 */
int get_service_status(const void *tfm);

#endif  // _CRYPTO_FIPS140_3_SERVICES_H
