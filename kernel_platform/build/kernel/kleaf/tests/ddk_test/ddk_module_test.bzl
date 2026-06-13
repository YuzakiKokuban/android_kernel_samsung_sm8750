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

"""Tests for `ddk_module`."""

load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/kernel/kleaf/impl:common_providers.bzl", "ModuleSymversInfo")
load("//build/kernel/kleaf/impl:ddk/ddk_headers.bzl", "ddk_headers")
load("//build/kernel/kleaf/impl:ddk/ddk_module.bzl", "ddk_module")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:kernel_module.bzl", "kernel_module")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")
load("//build/kernel/kleaf/tests/utils:contain_lines_test.bzl", "contain_lines_test")
load(":ddk_headers_test.bzl", "check_ddk_headers_info")
load(":makefiles_test.bzl", "get_top_level_file")

def _ddk_module_test_impl(ctx):
    env = analysistest.begin(ctx)

    target_under_test = analysistest.target_under_test(env)

    module_symvers_restore_paths = target_under_test[ModuleSymversInfo].restore_paths.to_list()
    asserts.equals(
        env,
        1,
        len(module_symvers_restore_paths),
        "{} has multiple Module.symvers, expected 1".format(target_under_test.label.name),
    )
    module_symvers_restore_path = module_symvers_restore_paths[0]
    asserts.true(
        env,
        target_under_test.label.name in module_symvers_restore_path,
        "Module.symvers is restored to {}, but it must contain {} to distinguish with other modules in the same package".format(
            module_symvers_restore_path,
            target_under_test.label.name,
        ),
    )

    expected_inputs = sets.make(ctx.files.expected_inputs)

    action = None
    mnemonic = "KernelModule"
    if ctx.attr._config_is_local[BuildSettingInfo].value:
        mnemonic += "ProcessWrapperSandbox"

    for a in target_under_test.actions:
        if a.mnemonic == mnemonic:
            action = a
    asserts.true(env, action, "Can't find action with mnemonic KernelModule")

    inputs = sets.make(action.inputs.to_list())
    asserts.true(
        env,
        sets.is_subset(expected_inputs, inputs),
        "Missing inputs to action {}".format(
            sets.to_list(sets.difference(expected_inputs, inputs)),
        ),
    )

    if len(ctx.files.unexpected_inputs) > 0:
        unexpected_inputs = sets.make(ctx.files.unexpected_inputs)
        asserts.false(
            env,
            sets.is_subset(unexpected_inputs, inputs),
            "Unexpected inputs to action {}".format(
                sets.to_list(sets.intersection(unexpected_inputs, inputs)),
            ),
        )

    check_ddk_headers_info(ctx, env)

    return analysistest.end(env)

ddk_module_test = analysistest.make(
    impl = _ddk_module_test_impl,
    attrs = {
        "expected_inputs": attr.label_list(allow_files = True),
        "unexpected_inputs": attr.label_list(allow_files = True),
        "expected_includes": attr.string_list(),
        "expected_hdrs": attr.label_list(allow_files = [".h"]),
        "_config_is_local": attr.label(
            default = "//build/kernel/kleaf:config_local",
        ),
    },
)

def _ddk_module_test_make(
        name,
        expected_inputs = None,
        unexpected_inputs = None,
        expected_hdrs = None,
        expected_includes = None,
        **kwargs):
    ddk_module(
        name = name + "_module",
        out = name + ".ko",
        tags = ["manual"],
        **kwargs
    )

    ddk_module_test(
        name = name,
        target_under_test = name + "_module",
        expected_inputs = expected_inputs,
        unexpected_inputs = unexpected_inputs,
        expected_hdrs = expected_hdrs,
        expected_includes = expected_includes,
    )

def _conditional_srcs_test(
        name,
        kernel_build):
    ddk_module(
        name = name + "_module",
        kernel_build = kernel_build,
        out = name + "_module.ko",
        conditional_srcs = {
            "CONFIG_A": {
                True: ["cond_srcs/a_y.c"],
                False: ["cond_srcs/a_n.c"],
            },
        },
        tags = ["manual"],
    )

    get_top_level_file(
        name = name + "_kbuild",
        filename = "Kbuild",
        target = name + "_module_makefiles",
        tags = ["manual"],
    )

    write_file(
        name = name + "_expected",
        out = name + "_expected/Kbuild",
        content = [
            "{}_module-$(CONFIG_A) += cond_srcs/a_y.o".format(name),
            "ifeq ($(CONFIG_A),)",
            "{}_module-y += cond_srcs/a_n.o".format(name),
            "endif # ifeq ($(CONFIG_A),)",
        ],
    )

    contain_lines_test(
        name = name,
        expected = name + "_expected",
        actual = name + "_kbuild",
        order = True,
    )

def ddk_module_test_suite(name):
    """Tests for `ddk_module`.

    Args:
        name: name of the test suite."""

    kernel_build(
        name = name + "_kernel_build",
        build_config = "build.config.fake",
        outs = ["vmlinux"],
        tags = ["manual"],
    )

    kernel_module(
        name = name + "_legacy_module_a",
        kernel_build = name + "_kernel_build",
        tags = ["manual"],
    )

    kernel_module(
        name = name + "_legacy_module_b",
        kernel_build = name + "_kernel_build",
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_self",
        out = name + "_self.ko",
        srcs = ["self.c"],
        kernel_build = name + "_kernel_build",
        tags = ["manual"],
    )

    ddk_module(
        name = name + "_base",
        out = name + "_base.ko",
        srcs = ["base.c"],
        kernel_build = name + "_kernel_build",
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_headers",
        includes = ["include"],
        hdrs = ["include/subdir.h"],
        tags = ["manual"],
    )

    tests = []

    _ddk_module_test_make(
        name = name + "_simple",
        srcs = ["self.c"],
        kernel_build = name + "_kernel_build",
    )
    tests.append(name + "_simple")

    _ddk_module_test_make(
        name = name + "_dep",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        deps = [name + "_base"],
    )
    tests.append(name + "_dep")

    _ddk_module_test_make(
        name = name + "_dep2",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        deps = [name + "_base", name + "_self"],
    )
    tests.append(name + "_dep2")

    _ddk_module_test_make(
        name = name + "_local_headers",
        srcs = ["dep.c", "include/subdir.h"],
        kernel_build = name + "_kernel_build",
        expected_inputs = ["include/subdir.h"],
    )
    tests.append(name + "_local_headers")

    _ddk_module_test_make(
        name = name + "_external_headers",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        deps = [name + "_headers"],
        expected_inputs = ["include/subdir.h"],
    )
    tests.append(name + "_external_headers")

    _ddk_module_test_make(
        name = name + "_depend_on_legacy_module",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        deps = [name + "_legacy_module_a"],
    )
    tests.append(name + "_depend_on_legacy_module")

    _ddk_module_test_make(
        name = name + "_generate_btf",
        srcs = ["self.c"],
        kernel_build = name + "_kernel_build",
        generate_btf = True,
        expected_inputs = [name + "_kernel_build/vmlinux"],
    )
    tests.append(name + "_generate_btf")

    _ddk_module_test_make(
        name = name + "_no_generate_btf",
        srcs = ["self.c"],
        kernel_build = name + "_kernel_build",
        generate_btf = False,
        unexpected_inputs = [name + "_kernel_build/vmlinux"],
    )
    tests.append(name + "_no_generate_btf")

    ddk_module(
        name = name + "_depend_on_legacy_modules_in_the_same_package_module",
        out = name + "_depend_on_legacy_modules_in_the_same_package_module.ko",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        deps = [name + "_legacy_module_a", name + "_legacy_module_b"],
        tags = ["manual"],
    )
    failure_test(
        name = name + "_depend_on_legacy_modules_in_the_same_package",
        target_under_test = name + "_depend_on_legacy_modules_in_the_same_package_module",
        error_message_substrs = [
            "Conflicting dependencies",
            name + "_legacy_module_a",
            name + "_legacy_module_b",
        ],
    )
    tests.append(name + "_depend_on_legacy_modules_in_the_same_package")

    _ddk_module_test_make(
        name = name + "_exported_headers",
        srcs = ["dep.c"],
        kernel_build = name + "_kernel_build",
        hdrs = ["include/subdir.h"],
        includes = ["include"],
        expected_inputs = ["include/subdir.h"],
        expected_hdrs = ["include/subdir.h"],
        expected_includes = [native.package_name() + "/include"],
    )
    tests.append(name + "_exported_headers")

    ddk_module(
        name = name + "_no_out_module",
        tags = ["manual"],
        kernel_build = name + "_kernel_build",
    )
    failure_test(
        name = name + "_no_out",
        target_under_test = name + "_no_out_module",
        error_message_substrs = ["out is not specified."],
    )
    tests.append(name + "_no_out")

    _conditional_srcs_test(
        name = name + "_conditional_srcs_test",
        kernel_build = name + "_kernel_build",
    )
    tests.append(name + "_conditional_srcs_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
