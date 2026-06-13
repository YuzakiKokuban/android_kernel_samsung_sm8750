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

"""Test that, if a flag is specified, and --lto is not none or unspecified, fail."""

load("//build/kernel/kleaf:constants.bzl", "LTO_VALUES")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")

def _lto_test_for_flag_common(
        name,
        flag,
        flag_values,
        check_fail_action_message_test_rule,
        expect_failure_message,
        defconfig_fragment = None,
        defconfig_fragment_suffix = None):
    """Instantiates multiple tests for different combinations of `flag` and LTO.

    Exactly one of `defconfig_fragment` or `defconfig_fragment_suffix` must be set.

    Args:
        name: name of test suite
        flag: flag under test
        flag_values: possible values of `flag`
        check_fail_action_message_test_rule: rule returned by `_make_check_fail_action_message_test`
        expect_failure_message: error message if LTO conflicts with `flag`
        defconfig_fragment: the `fail_action` target.
        defconfig_fragment_suffix: the suffix after the name of `kernel_build` of the name of
            the `fail_action` target.
    """
    tests = []
    kernel_build(
        name = name + "_subject",
        tags = ["manual"],
        build_config = Label("//common:build.config.gki.aarch64"),
        outs = [],
        make_goals = ["FAKE_MAKE_GOALS"],
    )
    for flag_value in flag_values:
        for lto in LTO_VALUES:
            expect_failure = flag_value and lto not in ("default", "none")
            test_name = "{}_{}_{}_lto_{}".format(name, native.package_relative_label(flag).name, flag_value, lto)

            expected_message = expect_failure_message if expect_failure else ""
            check_fail_action_message_test_rule(
                name = test_name,
                expected_message = expected_message,
                defconfig_fragment = defconfig_fragment if defconfig_fragment else name + "_subject" + defconfig_fragment_suffix,
                config_settings = {
                    flag: str(flag_value),
                    Label("//build/kernel/kleaf:lto"): lto,
                },
            )
            tests.append(test_name)
    native.test_suite(
        name = name,
        tests = tests,
    )

def _check_fail_action_message_transition_impl(_settings, attr):
    # attr.config_settings: dict[Label, str]
    # return value: dict[str, Any]
    transformed = {}
    for key, value in attr.config_settings.items():
        if value in ("True", "False"):
            value = value == "True"
        transformed[str(key)] = value
    transformed[str(Label("//build/kernel/kleaf/impl:fail_action_write_to_file"))] = True
    return transformed

def _check_fail_action_message_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    expected = ctx.actions.declare_file("{}/expected.txt".format(ctx.attr.name))
    ctx.actions.write(output = expected, content = ctx.attr.expected_message)

    actual = None
    if ctx.files.defconfig_fragment:
        if len(ctx.files.defconfig_fragment) != 1:
            fail("{}: {} has multiple files".format(ctx.label, ctx.attr.defconfig_fragment))
        actual = ctx.files.defconfig_fragment[0]
    if not actual:
        # file_selector successfully resolved to empty_filegroup, which is equivalent to
        # having an empty defconfig file.
        actual = ctx.actions.declare_file("{}/actual.txt".format(ctx.attr.name))
        ctx.actions.write(output = actual, content = "")

    exec = ctx.actions.declare_file("{}/test.sh".format(ctx.attr.name))

    direct_runfiles = []
    transitive_runfiles = None
    script = "#!/bin/bash -e"
    if ctx.attr.expected_message:
        # Expecting failure
        transitive_runfiles = hermetic_tools.deps
        direct_runfiles += [expected, actual]
        script = hermetic_tools.run_setup + """
                if ! diff -q {expected} {actual}; then
                    echo "ERROR: expected error message different from actual file." >&2
                    diff {expected} {actual} >&2
                    exit 1
                fi
            """.format(
            expected = expected.short_path,
            actual = actual.short_path,
        )
    else:
        # Expecting success
        if actual.basename == "expected_message.txt":
            transitive_runfiles = hermetic_tools.deps
            direct_runfiles.append(actual)
            script = hermetic_tools.run_setup + """
                echo "ERROR: expected success, but got" >&2
                cat {actual} >&2
                exit 1
            """.format(
                actual = actual.short_path,
            )

    ctx.actions.write(
        output = exec,
        content = script,
        is_executable = True,
    )

    runfiles = ctx.runfiles(files = direct_runfiles, transitive_files = transitive_runfiles)
    return DefaultInfo(executable = exec, runfiles = runfiles)

def _make_check_fail_action_message_test(flag):
    """Creates a rule() for the given flag under test."""
    the_transition = transition(
        implementation = _check_fail_action_message_transition_impl,
        inputs = [],
        outputs = [
            str(flag),
            str(Label("//build/kernel/kleaf:lto")),
            str(Label("//build/kernel/kleaf/impl:fail_action_write_to_file")),
        ],
    )

    return rule(
        implementation = _check_fail_action_message_impl,
        attrs = {
            "expected_message": attr.string(),
            "defconfig_fragment": attr.label(allow_files = True),
            "config_settings": attr.label_keyed_string_dict(),
            "_allowlist_function_transition": attr.label(
                default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
            ),
        },
        toolchains = [hermetic_toolchain.type],
        cfg = the_transition,
        test = True,
    )

_debug_lto_test_check_fail_action_message_test = _make_check_fail_action_message_test(
    flag = Label("//build/kernel/kleaf:debug"),
)

def debug_lto_test(name):
    _lto_test_for_flag_common(
        name = name,
        flag = Label("//build/kernel/kleaf:debug"),
        flag_values = (True, False),
        check_fail_action_message_test_rule = _debug_lto_test_check_fail_action_message_test,
        expect_failure_message = "--debug requires --lto=none or default.",
        defconfig_fragment = Label("//build/kernel/kleaf/impl/defconfig:debug"),
    )

_kasan_lto_test_check_fail_action_message_test = _make_check_fail_action_message_test(
    flag = Label("//build/kernel/kleaf:kasan"),
)

def kasan_lto_test(name):
    _lto_test_for_flag_common(
        name = name,
        flag = Label("//build/kernel/kleaf:kasan"),
        flag_values = (True, False),
        check_fail_action_message_test_rule = _kasan_lto_test_check_fail_action_message_test,
        expect_failure_message = "--kasan requires --lto=none or default.",
        defconfig_fragment_suffix = "_defconfig_fragment_sanitizer",
    )

_kasan_sw_tags_lto_test_check_fail_action_message_test = _make_check_fail_action_message_test(
    flag = Label("//build/kernel/kleaf:kasan_sw_tags"),
)

def kasan_sw_tags_lto_test(name):
    _lto_test_for_flag_common(
        name = name,
        flag = Label("//build/kernel/kleaf:kasan_sw_tags"),
        flag_values = (True, False),
        check_fail_action_message_test_rule = _kasan_sw_tags_lto_test_check_fail_action_message_test,
        expect_failure_message = "--kasan_sw_tags requires --lto=none or default.",
        defconfig_fragment_suffix = "_defconfig_fragment_sanitizer",
    )

_kasan_generic_lto_test_check_fail_action_message_test = _make_check_fail_action_message_test(
    flag = Label("//build/kernel/kleaf:kasan_generic"),
)

def kasan_generic_lto_test(name):
    _lto_test_for_flag_common(
        name = name,
        flag = Label("//build/kernel/kleaf:kasan_generic"),
        flag_values = (True, False),
        check_fail_action_message_test_rule = _kasan_generic_lto_test_check_fail_action_message_test,
        expect_failure_message = "--kasan_generic requires --lto=none or default.",
        defconfig_fragment_suffix = "_defconfig_fragment_sanitizer",
    )

_kcsan_lto_test_check_fail_action_message_test = _make_check_fail_action_message_test(
    flag = Label("//build/kernel/kleaf:kcsan"),
)

def kcsan_lto_test(name):
    _lto_test_for_flag_common(
        name = name,
        flag = Label("//build/kernel/kleaf:kcsan"),
        flag_values = (True, False),
        check_fail_action_message_test_rule = _kcsan_lto_test_check_fail_action_message_test,
        expect_failure_message = "--kcsan requires --lto=none or default.",
        defconfig_fragment_suffix = "_defconfig_fragment_sanitizer",
    )
