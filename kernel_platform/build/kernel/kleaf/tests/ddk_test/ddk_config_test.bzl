# Copyright (C) 2023 The Android Open Source Project
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

"""Test expectations on `ddk_config`.

Require `//common` package.
"""

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/kernel/kleaf:kernel.bzl", "ddk_module", "kernel_build")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load("//build/kernel/kleaf/impl:utils.bzl", "utils")
load("//build/kernel/kleaf/tests/utils:contain_lines_test.bzl", "contain_lines_test")

def _get_config_impl(ctx):
    out_dir = utils.find_file(
        name = "out_dir",
        files = ctx.files.ddk_config,
        what = "{}: ddk_config outputs".format(ctx.attr.ddk_config.label),
    )

    out = ctx.actions.declare_file("{}/.config".format(ctx.label.name))
    hermetic_tools = hermetic_toolchain.get(ctx)
    command = hermetic_tools.setup + """
        cp -pL {out_dir}/.config {out}
    """.format(
        out_dir = out_dir.path,
        out = out.path,
    )

    ctx.actions.run_shell(
        inputs = [out_dir],
        outputs = [out],
        command = command,
        tools = hermetic_tools.deps,
        mnemonic = "GetDdkConfigFile",
        progress_message = "Getting .config {}".format(ctx.label),
    )

    return DefaultInfo(files = depset([out]))

_get_config = rule(
    implementation = _get_config_impl,
    attrs = {
        "ddk_config": attr.label(),
    },
    toolchains = [hermetic_toolchain.type],
)

def ddk_config_test_suite(name):
    """Defines analysis test for `ddk_config`.

    Args:
        name: Name for this test suite.
    """

    tests = []

    native.filegroup(
        name = "fake_defconfig_fragment",
        srcs = ["defconfig.fragment"],
        visibility = ["//visibility:public"],
    )

    kernel_build(
        name = name + "_aarch64_kernel_build",
        srcs = ["//common:kernel_aarch64_sources"],
        arch = "arm64",
        build_config = "//common:build.config.gki.aarch64",
        outs = [],
        make_goals = ["vmlinux"],
        ddk_module_defconfig_fragments = [
            "fake_defconfig_fragment",
        ],
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_ddk_module",
        srcs = ["base.c"],
        out = name + "_ddk_module.ko",
        kernel_build = name + "_aarch64_kernel_build",
        tags = ["manual"],
    )

    _get_config(
        name = name + "_actual",
        ddk_config = name + "_ddk_module_config",
        tags = ["manual"],
    )

    write_file(
        name = name + "_expected",
        out = name + "_out_dir/.config",
        # It has to be a valid config.
        content = ["# CONFIG_MODULE_SIG_ALL is not set", ""],
        tags = ["manual"],
    )

    contain_lines_test(
        name = name + "_contain_lines_test",
        expected = name + "_expected",
        actual = name + "_actual",
    )
    tests.append(name + "_contain_lines_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
