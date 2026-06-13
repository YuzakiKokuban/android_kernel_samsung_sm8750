# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf/impl:ddk/ddk_headers.bzl", "DdkHeadersInfo", "ddk_headers")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")

def check_ddk_headers_info(ctx, env):
    """Check that the target implements DdkHeadersInfo with the expected `includes` and `hdrs`."""
    target_under_test = analysistest.target_under_test(env)

    # Check content + ordering of include dirs, so do list comparison.
    asserts.equals(
        env,
        ctx.attr.expected_includes,
        target_under_test[DdkHeadersInfo].includes.to_list(),
    )
    asserts.set_equals(
        env,
        sets.make(ctx.files.expected_hdrs),
        sets.make(target_under_test[DdkHeadersInfo].files.to_list()),
    )

def _good_includes_test_impl(ctx):
    env = analysistest.begin(ctx)
    check_ddk_headers_info(ctx, env)
    return analysistest.end(env)

_good_includes_test = analysistest.make(
    impl = _good_includes_test_impl,
    attrs = {
        "expected_includes": attr.string_list(),
        "expected_hdrs": attr.label_list(allow_files = [".h"]),
    },
)

def _ddk_headers_good_includes_test(
        name,
        includes,
        expected_includes,
        hdrs = None,
        expected_hdrs = None):
    ddk_headers(
        name = name + "_headers",
        includes = includes,
        hdrs = hdrs,
        tags = ["manual"],
    )
    _good_includes_test(
        name = name,
        target_under_test = name + "_headers",
        expected_includes = expected_includes,
        expected_hdrs = expected_hdrs,
    )

def _ddk_headers_bad_includes_test(name, includes, error_message):
    ddk_headers(
        name = name + "_headers",
        includes = includes,
        tags = ["manual"],
    )
    failure_test(
        name = name,
        target_under_test = name + "_headers",
        error_message_substrs = [error_message],
    )

def ddk_headers_test_suite(name):
    """Defines analysis test for `ddk_headers`."""

    tests = []

    _ddk_headers_good_includes_test(
        name = name + "_self",
        includes = ["."],
        expected_includes = [native.package_name()],
        hdrs = ["self.h"],
        expected_hdrs = ["self.h"],
    )
    tests.append(name + "_self")

    _ddk_headers_good_includes_test(
        name = name + "_subdir",
        includes = ["include"],
        expected_includes = ["{}/include".format(native.package_name())],
        hdrs = ["include/subdir.h"],
        expected_hdrs = ["include/subdir.h"],
    )
    tests.append(name + "_subdir")

    _ddk_headers_good_includes_test(
        name = name + "_subdir_subdir",
        includes = ["include/foo"],
        expected_includes = ["{}/include/foo".format(native.package_name())],
        hdrs = ["include/foo/foo.h"],
        expected_hdrs = ["include/foo/foo.h"],
    )
    tests.append(name + "_subdir_subdir")

    _ddk_headers_good_includes_test(
        name = name + "_multiple",
        includes = [".", "include", "include/foo"],
        expected_includes = [
            native.package_name(),
            "{}/include".format(native.package_name()),
            "{}/include/foo".format(native.package_name()),
        ],
        hdrs = ["self.h", "include/subdir.h", "include/foo/foo.h"],
        expected_hdrs = ["self.h", "include/subdir.h", "include/foo/foo.h"],
    )
    tests.append(name + "_multiple")

    _ddk_headers_bad_includes_test(
        name = name + "_prefix_dot",
        includes = ["./foo"],
        error_message = "not normalized to foo",
    )
    tests.append(name + "_prefix_dot")

    _ddk_headers_bad_includes_test(
        name = name + "_trailing_slash",
        includes = ["foo/"],
        error_message = "not normalized to foo",
    )
    tests.append(name + "_trailing_slash")

    _ddk_headers_bad_includes_test(
        name = name + "_absolute",
        includes = ["/foo"],
        error_message = "Absolute directories not allowed",
    )
    tests.append(name + "_absolute")

    _ddk_headers_bad_includes_test(
        name = name + "_parent_include",
        includes = [".."],
        error_message = "Invalid include directory",
    )
    tests.append(name + "_parent_include")

    _ddk_headers_bad_includes_test(
        name = name + "_parent_self_include",
        includes = ["../" + paths.basename(native.package_name())],
        error_message = "Invalid include directory",
    )
    tests.append(name + "_parent_self_include")

    ddk_headers(
        name = name + "_base_headers",
        includes = ["include/base"],
        hdrs = ["include/base/base.h"],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_foo_headers",
        includes = ["include/foo"],
        hdrs = ["include/foo/foo.h"],
        tags = ["manual"],
    )

    _ddk_headers_good_includes_test(
        name = name + "_transitive",
        includes = ["include/transitive"],
        hdrs = [name + "_base_headers", "include/transitive/transitive.h"],
        expected_includes = [
            # do not sort
            # First, includes
            "{}/include/transitive".format(native.package_name()),
            # Then, hdrs
            "{}/include/base".format(native.package_name()),
        ],
        expected_hdrs = ["include/base/base.h", "include/transitive/transitive.h"],
    )
    tests.append(name + "_transitive")

    _ddk_headers_good_includes_test(
        name = name + "_include_ordering",
        includes = [
            # do not sort
            "b",
            "a",
            "c",
        ],
        hdrs = [
            # do not sort
            name + "_foo_headers",
            name + "_base_headers",
        ],
        expected_includes = [
            # do not sort
            # First, includes
            "{}/b".format(native.package_name()),
            "{}/a".format(native.package_name()),
            "{}/c".format(native.package_name()),
            # Then, hdrs
            "{}/include/foo".format(native.package_name()),
            "{}/include/base".format(native.package_name()),
        ],
        expected_hdrs = [
            "include/base/base.h",
            "include/foo/foo.h",
        ],
    )
    tests.append(name + "_include_ordering")

    native.test_suite(
        name = name,
        tests = tests,
    )
