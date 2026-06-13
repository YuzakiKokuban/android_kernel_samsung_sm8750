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

"""Tests --debug_modpost_warn."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//build/kernel/kleaf/impl:ddk/ddk_module.bzl", "ddk_module")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:kernel_module.bzl", "kernel_module")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")

def _modpost_warn_module_test_impl(ctx):
    env = analysistest.begin(ctx)
    mnemonic = "KernelModule"
    if ctx.attr.is_ddk_test:
        if ctx.attr._config_is_local[BuildSettingInfo].value:
            mnemonic += "ProcessWrapperSandbox"
    action = test_utils.find_action(env, mnemonic)
    script = test_utils.get_shell_script(env, action)

    asserts.true(
        env,
        "export KBUILD_MODPOST_WARN=1" in script,
        "Can't find KBUILD_MODPOST_WARN=1 in script",
    )

    log_file = test_utils.find_output(action, "make_stderr.txt")
    asserts.true(env, log_file, "Cannot find make_stderr.txt from output")
    asserts.true(env, not log_file.is_directory)

    return analysistest.end(env)

_modpost_warn_module_test = analysistest.make(
    impl = _modpost_warn_module_test_impl,
    attrs = {
        "_config_is_local": attr.label(
            default = "//build/kernel/kleaf:config_local",
        ),
        "is_ddk_test": attr.bool(),
    },
    config_settings = {
        str(Label("//build/kernel/kleaf:debug_modpost_warn")): True,
    },
)

def debug_modpost_warn_test(name):
    """Tests --debug_modpost_warn.

    Args:
        name: name of the test suite.
    """

    # Test setup

    kernel_build(
        name = name + "_kernel",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    kernel_module(
        name = name + "_kernel_module",
        kernel_build = name + "_kernel",
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_ddk_module",
        out = name + "_ddk_module.ko",
        kernel_build = name + "_kernel",
        srcs = ["foo.c"],
        tags = ["manual"],
    )

    tests = []

    _modpost_warn_module_test(
        name = name + "_kernel_module_modpost_warn_test",
        target_under_test = name + "_kernel_module",
    )
    tests.append(name + "_kernel_module_modpost_warn_test")

    _modpost_warn_module_test(
        name = name + "_ddk_module_modpost_warn_test",
        target_under_test = name + "_ddk_module",
        is_ddk_test = True,
    )
    tests.append(name + "_ddk_module_modpost_warn_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
