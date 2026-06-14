// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/test.h>
#include "proca_identity.h"
#include "test_helpers.h"
#include "proca_test_certificate.h"

static void init_proca_identity_null_test(struct kunit *test)
{
	int rc;

	rc = init_proca_identity(NULL, NULL, NULL, 0, NULL);

	KUNIT_EXPECT_EQ(test, rc, -EINVAL);
}

static void init_proca_identity_test(struct kunit *test)
{
	int rc;
	struct proca_certificate parsed_cert;
	DECLARE_NEW(test, struct file, p_file);
	DECLARE_NEW(test, struct proca_identity, pi);
	DECLARE_NEW(test, char, cert_val);
	memcpy(cert_val, "test", sizeof("test"));

	rc = init_certificate_validation_hash();
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = parse_proca_certificate(pa_cert, sizeof(pa_cert), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = init_proca_identity(pi, p_file,
				 &cert_val, sizeof(cert_val), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);
}

static void proca_identity_copy_test(struct kunit *test)
{
	int rc;
	struct proca_certificate parsed_cert;
	DECLARE_NEW(test, struct file, p_file);
	DECLARE_NEW(test, struct proca_identity, pi_src);
	DECLARE_NEW(test, struct proca_identity, pi_dst);
	DECLARE_NEW(test, char, pi_src_val);
	DECLARE_NEW(test, char, pi_dst_val);
	memcpy(pi_src_val, "pi_src", sizeof("pi_src"));
	memcpy(pi_dst_val, "pi_dst", sizeof("pi_dst"));

	rc = init_certificate_validation_hash();
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = parse_proca_certificate(pa_cert, sizeof(pa_cert), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = parse_proca_certificate(pa_cert, sizeof(pa_cert), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = init_proca_identity(pi_src, p_file,
				 &pi_src_val, sizeof(pi_src_val), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = init_proca_identity(pi_dst, p_file,
				 &pi_dst_val, sizeof(pi_dst_val), &parsed_cert);
	KUNIT_EXPECT_EQ(test, rc, 0);

	rc = proca_identity_copy(pi_dst, pi_src);
	KUNIT_EXPECT_EQ(test, rc, 0);
}

static int proca_identity_test_init(struct kunit *test)
{
	return 0;
}

static void proca_identity_test_exit(struct kunit *test)
{
	return;
}

static struct kunit_case security_proca_test_cases[] = {
	KUNIT_CASE(init_proca_identity_null_test),
	KUNIT_CASE(init_proca_identity_test),
	KUNIT_CASE(proca_identity_copy_test),
	{},
};

static struct kunit_suite proca_identity_test = {
	.name = "proca-identity-test",
	.init = proca_identity_test_init,
	.exit = proca_identity_test_exit,
	.test_cases = security_proca_test_cases,
};
kunit_test_suites(&proca_identity_test);

MODULE_LICENSE("GPL v2");
