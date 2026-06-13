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

"""Test ordering of unpacking UAPI header archives."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/kernel/kleaf/impl:common_providers.bzl", "KernelBuildUapiInfo", "KernelModuleInfo")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:kernel_filegroup.bzl", "kernel_filegroup")
load("//build/kernel/kleaf/impl:kernel_module.bzl", "kernel_module")
load("//build/kernel/kleaf/impl:merged_kernel_uapi_headers.bzl", "merged_kernel_uapi_headers")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")

def _find_extract_command(env, target, commands):
    if KernelBuildUapiInfo in target:
        file_name = target.label.name + "_uapi_headers/kernel-uapi-headers.tar.gz"
    elif KernelModuleInfo in target:
        file_name = target.label.name + "/kernel-uapi-headers.tar.gz_staging"
    else:
        asserts.true(env, False, "Unrecognized target {}".format(target.label.name))
        return None

    for index, command in enumerate(commands):
        if file_name in command:
            return index

    asserts.true(env, False, "Can't find command to extract {}. commands: \n{}".format(
        file_name,
        "\n".join(commands),
    ))
    return None

def _assert_acending(env, lst, commands_text):
    for prev_item, next_item in zip(lst, lst[1:]):
        asserts.true(
            env,
            prev_item.index < next_item.index,
            "The extraction of {} should be earlier than {}, but it is not ({} >= {}). commands: \n{}".format(
                prev_item.target.label.name,
                next_item.target.label.name,
                prev_item.index,
                next_item.index,
                commands_text,
            ),
        )

def _extract_order_test_impl(ctx):
    env = analysistest.begin(ctx)

    action = test_utils.find_action(env, "MergedKernelUapiHeaders")
    script = test_utils.get_shell_script(env, action)
    commands = script.split("\n")

    command_indices = [
        struct(target = target, index = _find_extract_command(env, target, commands))
        for target in ctx.attr.expect_extract_order
    ]
    _assert_acending(env, command_indices, script)

    return analysistest.end(env)

_extract_order_test = analysistest.make(
    impl = _extract_order_test_impl,
    attrs = {
        "expect_extract_order": attr.label_list(),
    },
)

def _make_order_test(name, expect_extract_order, **kwargs):
    merged_kernel_uapi_headers(
        name = name + "_merged_kernel_uapi_headers",
        tags = ["manual"],
        **kwargs
    )

    _extract_order_test(
        name = name,
        target_under_test = name + "_merged_kernel_uapi_headers",
        expect_extract_order = expect_extract_order,
    )

def order_test(name):
    """Test ordering of unpacking UAPI header archives.

    Args:
        name: name of test
    """
    tests = []

    kernel_build(
        name = name + "_base",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    native.filegroup(
        name = name + "_base_modules_staging_archive",
        srcs = [name + "_base"],
        output_group = "modules_staging_archive",
        tags = ["manual"],
    )

    write_file(
        name = name + "_gki_info",
        out = name + "_gki_info/gki-info.txt",
        content = [
            "KERNEL_RELEASE=99.98.97",
            "",
        ],
    )

    kernel_filegroup(
        name = name + "_fg",
        srcs = [name + "_base"],
        deps = [
            name + "_base_modules_staging_archive",
        ],
        kernel_uapi_headers = name + "_base_uapi_headers",
        gki_artifacts = name + "_gki_info",
        target_platform = Label("//build/kernel/kleaf/impl:android_arm64"),
        exec_platform = Label("//build/kernel/kleaf/impl:linux_x86_64"),
        tags = ["manual"],
    )

    for base_kernel in (
        name + "_base",
        name + "_fg",
    ):
        kernel_build(
            name = base_kernel + "_device",
            base_kernel = base_kernel,
            build_config = "build.config.fake",
            outs = [],
            tags = ["manual"],
        )

        kernel_module(
            name = base_kernel + "_external_module",
            kernel_build = base_kernel + "_device",
            tags = ["manual"],
        )

        _make_order_test(
            name = base_kernel + "_base_device_kernel_test",
            kernel_build = base_kernel + "_device",
            expect_extract_order = [
                base_kernel + "_device",
                # Both _base and _fg has the same UAPI headers target from _base
                name + "_base",
            ],
        )
        tests.append(base_kernel + "_base_device_kernel_test")

        _make_order_test(
            name = base_kernel + "_module_test",
            kernel_build = base_kernel + "_device",
            kernel_modules = [base_kernel + "_external_module"],
            expect_extract_order = [
                base_kernel + "_external_module",
                base_kernel + "_device",
                # Both _base and _fg has the same UAPI headers target from _base
                name + "_base",
            ],
        )
        tests.append(base_kernel + "_module_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
