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

"""Test `kbuild_symtypes`."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")
load(":kernel_env_aspect.bzl", "KernelEnvAspectInfo", "kernel_env_aspect")

def _kbuild_symtypes_test_make_vars(ctx, env):
    kernel_build = analysistest.target_under_test(env)
    kernel_env = kernel_build[KernelEnvAspectInfo].kernel_env

    action = test_utils.find_action(env, "KernelEnv", kernel_env.actions)
    script = test_utils.get_shell_script(env, action)
    found_command = "KBUILD_SYMTYPES=1" in script

    asserts.equals(
        env,
        actual = found_command,
        expected = ctx.attr.expect_kbuild_symtypes,
        msg = "expect_kbuild_symtypes = {}, but KBUILD_SYMTYPES is {}found".format(ctx.attr.expect_kbuild_symtypes, "" if found_command else "not "),
    )

def _kbuild_symtypes_test_output(ctx, env):
    action = test_utils.find_action(env, "KernelBuild")
    symtypes_dir = test_utils.find_output(action, "symtypes")

    asserts.equals(
        env,
        actual = bool(symtypes_dir),
        expected = ctx.attr.expect_kbuild_symtypes,
        msg = "expect_kbuild_symtypes = {}, but {} symtypes/ directory".format(ctx.attr.expect_kbuild_symtypes, "found" if symtypes_dir else "not found"),
    )

    if symtypes_dir:
        asserts.true(env, symtypes_dir.is_directory)

# Check effect of kbuild_symtypes
def _kbuild_symtypes_test_impl(ctx):
    env = analysistest.begin(ctx)

    _kbuild_symtypes_test_make_vars(ctx, env)
    _kbuild_symtypes_test_output(ctx, env)

    return analysistest.end(env)

def _make_kbuild_symtypes_test(kbuild_symtypes_flag_value):
    return analysistest.make(
        impl = _kbuild_symtypes_test_impl,
        attrs = {
            "expect_kbuild_symtypes": attr.bool(),
        },
        config_settings = {
            str(Label("//build/kernel/kleaf:kbuild_symtypes")): kbuild_symtypes_flag_value,
        },
        extra_target_under_test_aspects = [kernel_env_aspect],
    )

kbuild_symtypes_flag_true_test = _make_kbuild_symtypes_test(True)
kbuild_symtypes_flag_false_test = _make_kbuild_symtypes_test(False)

def kernel_build_symtypes_test(name):
    """Define tests for `kbuild_symtypes`.

    Args:
      name: Name of this test suite.
    """
    tests = []

    for kbuild_symtypes in ("true", "false", "auto"):
        kernel_build(
            name = name + "_" + kbuild_symtypes + "_subject",
            tags = ["manual"],
            build_config = "//common:build.config.gki.aarch64",
            outs = [],
            kbuild_symtypes = kbuild_symtypes,
        )

        kbuild_symtypes_flag_true_test(
            name = name + "_" + kbuild_symtypes + "_flag_true",
            target_under_test = name + "_" + kbuild_symtypes + "_subject",
            expect_kbuild_symtypes = kbuild_symtypes in ("true", "auto"),
        )
        tests.append(name + "_" + kbuild_symtypes + "_flag_true")

        kbuild_symtypes_flag_false_test(
            name = name + "_" + kbuild_symtypes + "_flag_false",
            target_under_test = name + "_" + kbuild_symtypes + "_subject",
            expect_kbuild_symtypes = kbuild_symtypes == "true",
        )
        tests.append(name + "_" + kbuild_symtypes + "_flag_false")

    native.test_suite(
        name = name,
        tests = tests,
    )
