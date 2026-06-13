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

"""
Test `make_goals` attribute in `kernel_build` rule.
"""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")

# Check effect of strip_modules
def _make_goals_test_impl(ctx):
    env = analysistest.begin(ctx)
    action = test_utils.find_action(env, "KernelBuild")
    script = test_utils.get_shell_script(env, action)
    all_found = True
    any_found = False
    must_have_make_goals = ctx.attr.must_have_make_goals + ctx.attr._additional_make_goals
    for goal in must_have_make_goals:
        all_found = all_found and goal in script
    for goal in ctx.attr.must_not_have_make_goals:
        any_found = any_found or goal in script
    asserts.true(
        env,
        all_found,
        msg = "Not all [{}] goals were found!".format(
            ctx.attr.must_have_make_goals,
        ),
    )
    asserts.false(
        env,
        any_found,
        msg = "Some non-expected goals from [{}] were found!".format(
            ctx.attr.must_not_have_make_goals,
        ),
    )
    return analysistest.end(env)

def _create_make_goals_test(kgdb_value = False, force_vmlinux = False, additional_make_goals = []):
    return analysistest.make(
        impl = _make_goals_test_impl,
        config_settings = {
            str(Label("//build/kernel/kleaf:kgdb")): kgdb_value,
            str(Label("//build/kernel/kleaf/impl:force_add_vmlinux")): force_vmlinux,
        },
        attrs = {
            "must_have_make_goals": attr.string_list(allow_empty = True),
            "must_not_have_make_goals": attr.string_list(allow_empty = True),
            "_additional_make_goals": attr.string_list(
                allow_empty = True,
                default = additional_make_goals,
            ),
            "_config_is_local": attr.label(
                default = "//build/kernel/kleaf:config_local",
            ),
        },
    )

make_goals_test = _create_make_goals_test()
kgdb_make_goals_test = _create_make_goals_test(True, False, ["scripts_gdb"])
with_vmlinux_make_goals_test = _create_make_goals_test(False, True, ["vmlinux"])
kgdb_with_vmlinux_make_goals_test = _create_make_goals_test(True, True, ["vmlinux", "scripts_gdb"])

def kernel_build_make_goals_test(name):
    """Define tests for `make_goals`.

    Args:
      name: Name of this test suite.
    """
    tests = []

    # Test the fallback (MAKE_GOALS) from config file.
    kernel_build(
        name = name + "_build_from_config",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    # Test by setting the goals from kernel build rule.
    kernel_build(
        name = name + "_build_from_rule",
        build_config = "build.config.fake",
        make_goals = [
            "GOAL_FROM_RULE_1",
            "GOAL_FROM_RULE_2",
        ],
        outs = [],
        tags = ["manual"],
    )
    for suffix, test in [
        ("default", make_goals_test),
        ("kgdb_enable", kgdb_make_goals_test),
        ("with_vmlinux", with_vmlinux_make_goals_test),
        ("kgdb_enable_with_vmlinux", kgdb_with_vmlinux_make_goals_test),
    ]:
        test(
            name = name + "_goals_from_config_" + suffix,
            target_under_test = name + "_build_from_config",
            must_have_make_goals = ["${MAKE_GOALS}"],
            must_not_have_make_goals = [],
        )
        tests.append(name + "_goals_from_config_" + suffix)

        test(
            name = name + "_goals_from_rule_" + suffix,
            target_under_test = name + "_build_from_rule",
            must_have_make_goals = [
                "GOAL_FROM_RULE_1",
                "GOAL_FROM_RULE_2",
            ],
            must_not_have_make_goals = ["${MAKE_GOALS}"],
        )
        tests.append(name + "_goals_from_rule_" + suffix)

    native.test_suite(
        name = name,
        tests = tests,
    )
