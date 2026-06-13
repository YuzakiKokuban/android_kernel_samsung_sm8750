# Copyright (C) 2024 The Android Open Source Project
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
"""
Defines ddk_uapi_headers tests.
"""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf/impl:ddk/ddk_uapi_headers.bzl", "ddk_uapi_headers")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")

def check_ddk_uapi_headers_info(env):
    """Check that the number of outputs and the output's format are correct.

    Args:
        env: The test environment.
    """
    target_under_test = analysistest.target_under_test(env)
    output_files = target_under_test[DefaultInfo].files.to_list()
    output_file = output_files[0].basename
    num_output_files = len(output_files)

    asserts.equals(
        env,
        num_output_files,
        1,
        "Expected 1 output file, but found {} files".format(num_output_files),
    )

    asserts.true(
        env,
        output_file.endswith(".tar.gz"),
        "Expected GZIP compressed tarball for output, but found {}".format(output_file),
    )

def _good_uapi_headers_test_impl(ctx):
    env = analysistest.begin(ctx)
    check_ddk_uapi_headers_info(env)
    return analysistest.end(env)

_good_uapi_headers_test = analysistest.make(
    impl = _good_uapi_headers_test_impl,
)

def _ddk_uapi_headers_good_headers_test(
        name,
        srcs = None):
    kernel_build(
        name = name + "_kernel_build",
        build_config = "build.config.fake",
        outs = ["vmlinux"],
        tags = ["manual"],
    )

    ddk_uapi_headers(
        name = name + "_headers",
        srcs = srcs,
        out = "good_headers.tar.gz",
        kernel_build = name + "_kernel_build",
    )

    _good_uapi_headers_test(
        name = name,
        target_under_test = name + "_headers",
    )

def _ddk_uapi_headers_bad_headers_test(name, srcs):
    kernel_build(
        name = name + "_kernel_build",
        build_config = "build.config.fake",
        outs = ["vmlinux"],
        tags = ["manual"],
    )

    ddk_uapi_headers(
        name = name + "_bad_headers_out_file_name",
        srcs = srcs,
        out = "bad-headers.gz",
        kernel_build = name + "_kernel_build",
    )

    failure_test(
        name = name,
        target_under_test = name + "_bad_headers_out_file_name",
        error_message_substrs = ["out-file name must end with"],
    )

def ddk_uapi_headers_test_suite(name):
    """Defines analysis test for `ddk_uapi_headers`.

    Args:
        name: rule name
    """

    tests = []

    _ddk_uapi_headers_good_headers_test(
        name = name + "_good_headers_test",
        srcs = ["include/uapi/uapi.h"],
    )
    tests.append(name + "_good_headers_test")

    _ddk_uapi_headers_bad_headers_test(
        name = name + "_bad_headers_test",
        srcs = ["include/uapi/uapi.h"],
    )
    tests.append(name + "_bad_headers_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
