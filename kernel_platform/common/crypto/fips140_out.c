// SPDX-License-Identifier: GPL-2.0
//
// In Samsung R&D Institute Ukraine, LLC (SRUKR) under a contract between
// Samsung R&D Institute Ukraine, LLC (Kyiv, Ukraine)
// and "Samsung Electronics Co", Ltd (Seoul, Republic of Korea)
// Copyright: (c) Samsung Electronics Co, Ltd 2024. All rights reserved.
//

#include "fips140.h"

__section(".rodata")
const volatile uint8_t buildtime_crypto_hmac[FIPS_HMAC_SIZE] = {0};

__section(".rodata")
const volatile struct first_last integrity_crypto_addrs[FIPS_CRYPTO_ADDRS_SIZE] = {{0},};

__section(".rodata")
const volatile uint64_t crypto_buildtime_address = 10;
