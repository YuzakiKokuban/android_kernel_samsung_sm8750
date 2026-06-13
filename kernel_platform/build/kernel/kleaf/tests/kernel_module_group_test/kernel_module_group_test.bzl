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

"""Tests for kernel_module_group."""

load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load(
    "//build/kernel/kleaf/impl:common_providers.bzl",
    "KernelModuleInfo",
    "KernelModuleSetupInfo",
)
load("//build/kernel/kleaf/impl:ddk/ddk_headers.bzl", "DdkHeadersInfo")
load("//build/kernel/kleaf/impl:ddk/ddk_module.bzl", "ddk_module")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:kernel_module_group.bzl", "kernel_module_group")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")

def _assert_is_subset(env, a, b, msg = ""):
    """Asserts that set(a) is a subset of set(b)."""
    asserts.true(
        env,
        sets.is_subset(sets.make(a), sets.make(b)),
        "{} Expecting {} contains {}".format(msg, a, b),
    )

def _check_info_field_depset_merged(env, target, src, info, attr_name):
    """Asserts that target[info].<attr_name> is a superset of src[info].attr_name."""
    target_attr = getattr(target[info], attr_name)
    if type(target_attr) == type(depset()):
        target_attr = target_attr.to_list()

    src_attr = getattr(src[info], attr_name)
    if type(src_attr) == type(depset()):
        src_attr = src_attr.to_list()

    _assert_is_subset(
        env,
        src_attr,
        target_attr,
        "{} {} is not merged.".format(info, attr_name),
    )

def _kernel_module_group_test_impl(ctx):
    env = analysistest.begin(ctx)
    target = analysistest.target_under_test(env)

    # check that DefaultInfo is merged
    asserts.set_equals(
        env,
        sets.make(target.files.to_list()),
        sets.make(ctx.files.expected_srcs),
    )

    for src in ctx.attr.expected_srcs:
        # check that KernelModuleSetupInfo is merged
        asserts.true(
            src[KernelModuleSetupInfo].setup in target[KernelModuleSetupInfo].setup,
            "KernelModuleSetupInfo setup is not merged; expecting\n{}\n\n... in ...\n\n{}".format(
                src[KernelModuleSetupInfo].setup,
                target[KernelModuleSetupInfo].setup,
            ),
        )
        _check_info_field_depset_merged(env, target, src, KernelModuleSetupInfo, "inputs")

        # check that DdkHeadersInfo is merged
        _check_info_field_depset_merged(env, target, src, DdkHeadersInfo, "files")
        _check_info_field_depset_merged(env, target, src, DdkHeadersInfo, "includes")

        # check that KernelModuleInfo is merged
        _check_info_field_depset_merged(env, target, src, KernelModuleInfo, "modules_staging_dws_depset")
        _check_info_field_depset_merged(env, target, src, KernelModuleInfo, "kernel_uapi_headers_dws_depset")
        _check_info_field_depset_merged(env, target, src, KernelModuleInfo, "files")

    return analysistest.end(env)

_kernel_module_group_test = analysistest.make(
    impl = _kernel_module_group_test_impl,
    attrs = {
        "expected_srcs": attr.label_list(),
    },
)

def _good_test(
        name,
        srcs):
    kernel_module_group(
        name = name + "_module_group",
        srcs = srcs,
        tags = ["manual"],
    )

    _kernel_module_group_test(
        name = name,
        target_under_test = name + "_module_group",
        expected_srcs = srcs,
    )

def _bad_test(
        name,
        srcs,
        error_message):
    kernel_module_group(
        name = name + "_module_group",
        srcs = srcs,
        tags = ["manual"],
    )

    failure_test(
        name = name,
        target_under_test = name + "_module_group",
        error_message_substrs = [error_message],
    )

def kernel_module_group_test(name):
    """Tests for kernel_module_group.

    Args:
        name: name of the main test suite
    """

    # Test setup
    kernel_build(
        name = name + "_build_a",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    kernel_build(
        name = name + "_build_b",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_ddk_a1",
        out = name + "_ddk_a1.ko",
        kernel_build = name + "_build_a",
        srcs = [],
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_ddk_a2",
        out = name + "_ddk_a2.ko",
        kernel_build = name + "_build_a",
        srcs = [],
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_ddk_b",
        out = name + "_ddk_b.ko",
        kernel_build = name + "_build_b",
        srcs = [],
        tags = ["manual"],
    )

    tests = []

    _good_test(
        name = name + "_one",
        srcs = [name + "_ddk_a1"],
    )
    tests.append(name + "_one")

    _good_test(
        name = name + "_two",
        srcs = [
            name + "_ddk_a1",
            name + "_ddk_a2",
        ],
    )
    tests.append(name + "_two")

    _bad_test(
        name = name + "_bad",
        srcs = [
            name + "_ddk_a1",
            name + "_ddk_b",
        ],
        error_message = "They must refer to the same kernel_build.",
    )
    tests.append(name + "_bad")

    native.test_suite(
        name = name,
        tests = tests,
    )
