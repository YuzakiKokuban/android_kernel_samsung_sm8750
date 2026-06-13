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

"""
Test kernel_config against options (e.g. lto).
Require //common package.
"""

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/kernel/kleaf:kernel.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load("//build/kernel/kleaf/impl:utils.bzl", "utils")
load("//build/kernel/kleaf/tests/utils:contain_lines_test.bzl", "contain_lines_test")
load(":kernel_config_aspect.bzl", "KernelConfigAspectInfo", "kernel_config_aspect")

_ARCHS = (
    "aarch64",
    "x86_64",
    # b/264407394: gcov does not work with riscv64 because of conflict in CONFIG_CFI_CLANG
    # "riscv64",
)

_INTERESTING_FLAGS = (
    "//build/kernel/kleaf:gcov",
    "//build/kernel/kleaf:btf_debug_info",
    "//build/kernel/kleaf:debug",
    "//build/kernel/kleaf:kasan",
    "//build/kernel/kleaf:kasan_sw_tags",
    "//build/kernel/kleaf:kasan_generic",
    "//build/kernel/kleaf:kcsan",
)

def _flag_transition_impl(settings, attr):
    ret = dict(settings)
    for key, value in attr.flag_values.items():
        if value == "True":
            ret[key] = True
        elif value == "False":
            ret[key] = False
        else:
            ret[key] = value
    return ret

_flag_transition = transition(
    implementation = _flag_transition_impl,
    inputs = _INTERESTING_FLAGS,
    outputs = _INTERESTING_FLAGS,
)

def _get_config_impl(ctx):
    if not len(ctx.attr.kernel_build) == 1:
        fail("This test does not support multiple configurations yet")

    kernel_build = ctx.attr.kernel_build[0]
    kernel_config = kernel_build[KernelConfigAspectInfo].kernel_config
    out_dir = utils.find_file(
        name = "out_dir",
        files = kernel_config.files.to_list(),
        what = "{}: kernel_config outputs".format(kernel_build.label),
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
        mnemonic = "GetConfigFile",
        progress_message = "Getting .config {}".format(ctx.label),
    )

    return DefaultInfo(files = depset([out]))

_get_config = rule(
    implementation = _get_config_impl,
    attrs = {
        "kernel_build": attr.label(
            aspects = [kernel_config_aspect],
            cfg = _flag_transition,
        ),
        "flag_values": attr.string_dict(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    toolchains = [hermetic_toolchain.type],
)

def _transition_test_for_flag_values_configs(
        name,
        kernel_build,
        flag_values_configs):
    flag_values = {
        flag_value.flag: flag_value.value
        for flag_value in flag_values_configs.flag_values
    }

    _get_config(
        name = name + "_actual",
        kernel_build = kernel_build,
        flag_values = flag_values,
        tags = ["manual"],
    )

    write_file(
        name = name + "_expected",
        out = name + "_expected/.config",
        content = flag_values_configs.configs + [""],
        tags = ["manual"],
    )

    contain_lines_test(
        name = name,
        expected = name + "_expected",
        actual = name + "_actual",
    )

def _transition_test(
        name,
        kernel_build,
        expected):
    tests = []

    for expected_value_list in expected:
        for flag_values_configs in expected_value_list:
            test_name = name + _get_test_name_suffix(flag_values_configs)
            _transition_test_for_flag_values_configs(
                name = test_name,
                kernel_build = kernel_build,
                flag_values_configs = flag_values_configs,
            )
            tests.append(test_name)

    native.test_suite(
        name = name + "_single",
        tests = tests,
    )

    comb_tests = []
    for flag_values_configs in _combinations(expected):
        # flag_values_configs: list[_FlagValueConfig]
        test_name = name + "_comb_" + _get_test_name_suffix(flag_values_configs)
        _transition_test_for_flag_values_configs(
            name = test_name,
            kernel_build = kernel_build,
            flag_values_configs = flag_values_configs,
        )
        comb_tests.append(test_name)

    native.test_suite(
        name = name + "_comb",
        tests = comb_tests,
    )

    native.test_suite(
        name = name,
        tests = [
            name + "_single",
            name + "_comb",
        ],
    )

# buildifier: disable=name-conventions
_FlagValue = provider("flag value tuple", fields = ["flag", "value"])

# buildifier: disable=name-conventions
_FlagValuesConfigs = provider("Tuple of _FlagValue and configs", fields = ["flag_values", "configs"])

def _get_test_name_suffix(flag_values_configs):
    ret = []
    for flag_value in flag_values_configs.flag_values:
        ret.append(native.package_relative_label(flag_value.flag).name + "_" + flag_value.value)
    return "_".join(ret)

def kernel_defconfig_fragments_test(name):
    """Tests for various flags on `kernel_config`.

    Args:
        name: name of the test
    """

    tests = []

    for arch in _ARCHS:
        kernel_build_arch = arch
        kasan_sw_tags_flag_values_configs_list = []
        if kernel_build_arch == "aarch64":
            kernel_build_arch = "arm64"
            kasan_sw_tags_flag_values_configs_list = [
                _FlagValuesConfigs(
                    flag_values = [
                        _FlagValue(flag = "//build/kernel/kleaf:kasan", value = "False"),
                        _FlagValue(flag = "//build/kernel/kleaf:kasan_sw_tags", value = "True"),
                        _FlagValue(flag = "//build/kernel/kleaf:kasan_generic", value = "False"),
                        _FlagValue(flag = "//build/kernel/kleaf:kcsan", value = "False"),
                    ],
                    configs = ["CONFIG_KASAN_SW_TAGS=y"],
                ),
            ]

        name_arch = "{}_{}".format(name, arch)
        kernel_build(
            name = name_arch + "_kernel_build",
            srcs = ["//common:kernel_{}_sources".format(arch)],
            arch = kernel_build_arch,
            build_config = "//common:build.config.gki.{}".format(arch),
            outs = [],
            make_goals = ["Image"],
            tags = ["manual"],
        )

        _transition_test(
            name = name_arch + "_test",
            kernel_build = name_arch + "_kernel_build",
            expected = [
                [
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:gcov", value = "True")],
                        configs = ["CONFIG_GCOV_KERNEL=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:gcov", value = "False")],
                        configs = ["# CONFIG_GCOV_KERNEL is not set"],
                    ),
                ],
                [
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:btf_debug_info", value = "enable")],
                        configs = ["CONFIG_DEBUG_INFO_BTF=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:btf_debug_info", value = "disable")],
                        configs = ["# CONFIG_DEBUG_INFO_BTF is not set"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:btf_debug_info", value = "default")],
                        configs = [],
                    ),
                ],
                [
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:debug", value = "True")],
                        configs = ["CONFIG_DEBUG_BUGVERBOSE=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [_FlagValue(flag = "//build/kernel/kleaf:debug", value = "False")],
                        configs = [],
                    ),
                ],
                # --k*san are mutually exclusive
                [
                    _FlagValuesConfigs(
                        flag_values = [
                            _FlagValue(flag = "//build/kernel/kleaf:kasan", value = "True"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_sw_tags", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_generic", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kcsan", value = "False"),
                        ],
                        configs = ["CONFIG_KASAN=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [
                            _FlagValue(flag = "//build/kernel/kleaf:kasan", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_sw_tags", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_generic", value = "True"),
                            _FlagValue(flag = "//build/kernel/kleaf:kcsan", value = "False"),
                        ],
                        configs = ["CONFIG_KASAN_GENERIC=y", "CONFIG_KASAN_OUTLINE=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [
                            _FlagValue(flag = "//build/kernel/kleaf:kasan", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_sw_tags", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_generic", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kcsan", value = "True"),
                        ],
                        configs = ["CONFIG_KCSAN=y"],
                    ),
                    _FlagValuesConfigs(
                        flag_values = [
                            _FlagValue(flag = "//build/kernel/kleaf:kasan", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_sw_tags", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kasan_generic", value = "False"),
                            _FlagValue(flag = "//build/kernel/kleaf:kcsan", value = "False"),
                        ],
                        configs = [],
                    ),
                ] + kasan_sw_tags_flag_values_configs_list,
            ],
        )
        tests.append(name_arch + "_test")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _combinations(arr):
    """Generates combinations for _FlagValuesConfigs.

    Args:
        arr: a 2-D array of _FlagValuesConfigs

    Returns:
        An array of _FlagValuesConfigs, where each _FlagValuesConfigs represents a combination.
        Order is undefined but deterministic.
    """

    ret = []
    num_combinations = 1
    for row in arr:
        num_combinations *= len(row)

    for i in range(num_combinations):
        current_choices = _FlagValuesConfigs(flag_values = [], configs = [])
        for row in arr:
            current_choices = _add(current_choices, row[i % len(row)])
            i //= len(row)
        ret.append(current_choices)

    return ret

def _add(flag_values_configs_1, flag_values_configs_2):
    return _FlagValuesConfigs(
        flag_values = flag_values_configs_1.flag_values + flag_values_configs_2.flag_values,
        configs = flag_values_configs_1.configs + flag_values_configs_2.configs,
    )
