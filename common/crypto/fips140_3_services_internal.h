/* SPDX-License-Identifier: GPL-2.0 */
//
// In Samsung R&D Institute Ukraine, LLC (SRUKR) under a contract between
// Samsung R&D Institute Ukraine, LLC (Kyiv, Ukraine)
// and "Samsung Electronics Co", Ltd (Seoul, Republic of Korea)
// Copyright: (c) Samsung Electronics Co, Ltd 2024. All rights reserved.
//

#ifndef _CRYPTO_FIPS140_3_SERVICES_INTERNAL_H
#define _CRYPTO_FIPS140_3_SERVICES_INTERNAL_H

#include <linux/kernel.h>

enum approve_status {
	STATUS_NON_APPROVED = 0,
	STATUS_APPROVED = 1,
	STATUS_UNDEFINED,
};

/**
 * Calculate final status of the service based on associated properties
 * record.
 *
 * @tfm: crypto transformation handler.
 * Return: 1 if the service is in approved state, 0 otherwise.
 */
void calc_final_state_service_indicator(const void *tfm);

/**
 * Set the HMAC specific property based on |key_size|.
 * It is necessary to be deployed in crypto hash setkey implementation.
 *
 * @tfm: crypto transformation handler.
 * @key_size: size of HMAC key.
 */
void crypto_hmac_set_key_approve_status(const void *tfm,
					unsigned int key_size);

#endif  // _CRYPTO_FIPS140_3_SERVICES_INTERNAL_H
