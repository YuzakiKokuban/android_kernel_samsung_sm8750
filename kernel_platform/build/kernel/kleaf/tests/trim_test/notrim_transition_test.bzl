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

"""Tests `force_disable_trim` and `notrim_transition`."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//build/kernel/kleaf/impl:abi/abi_transitions.bzl", "abi_common_attrs", "notrim_transition")
load("//build/kernel/kleaf/impl:abi/base_kernel_utils.bzl", "base_kernel_utils")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load(
    ":trim_aspect.bzl",
    "TrimAspectInfo",
    "check_kernel_build_trim_attr",
    "trim_aspect",
)

_IgnoreBaseKernelInfo = provider(
    "Info that indicates the value of `//build/kernel/kleaf/impl:force_ignore_base_kernel`",
    fields = base_kernel_utils.config_settings_raw(),
)

def _ignore_base_kernel_aspect_impl(_target, ctx):
    attrs = base_kernel_utils.config_settings_raw()
    attr_val_map = {attr: getattr(ctx.rule.attr, attr)[BuildSettingInfo].value for attr in attrs}
    return _IgnoreBaseKernelInfo(**attr_val_map)

_ignore_base_kernel_aspect = aspect(
    implementation = _ignore_base_kernel_aspect_impl,
)

def _fake_extracted_symbols_impl(ctx):
    return [
        ctx.attr.kernel_build[TrimAspectInfo],
        ctx.attr.kernel_build[_IgnoreBaseKernelInfo],
    ]

_fake_extracted_symbols = rule(
    implementation = _fake_extracted_symbols_impl,
    attrs = {
        "kernel_build": attr.label(aspects = [trim_aspect, _ignore_base_kernel_aspect]),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    } | abi_common_attrs(),
    cfg = notrim_transition,
)

def _notrim_transition_analysis_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    check_kernel_build_trim_attr(
        env = env,
        expect_trim = False,
        target_trim_info = target_under_test[TrimAspectInfo],
    )
    asserts.true(
        env,
        target_under_test[_IgnoreBaseKernelInfo]._force_ignore_base_kernel,
        "force_ignore_base_kernel is False: {}".format(target_under_test.label),
    )
    return analysistest.end(env)

_notrim_transition_analysis_test = analysistest.make(
    impl = _notrim_transition_analysis_test_impl,
)

def notrim_transition_test(name):
    """Tests `force_disable_trim` and `notrim_transition`.

    Args:
        name: name of the test
    """

    tests = []
    for base_trim in (True, False):
        base_trim_str = "trim" if base_trim else "notrim"
        kernel_build(
            name = name + "_{}_base_build".format(base_trim_str),
            build_config = "build.config.kernel",
            outs = [],
            trim_nonlisted_kmi = base_trim,
            kmi_symbol_list = "symbol_list_base",
            tags = ["manual"],
        )

        _fake_extracted_symbols(
            name = name + "_{}_base_extracted_symbols".format(base_trim_str),
            kernel_build = name + "_{}_base_build".format(base_trim_str),
        )

        _notrim_transition_analysis_test(
            name = name + "_{}_base_test".format(base_trim_str),
            target_under_test = name + "_{}_base_extracted_symbols".format(base_trim_str),
        )
        tests.append(name + "_{}_base_test".format(base_trim_str))

        for device_trim in (True, False):
            device_trim_str = "trim" if device_trim else "notrim"

            kernel_build(
                name = name + "_{}_{}_device_build".format(base_trim_str, device_trim_str),
                build_config = "build.config.device",
                base_kernel = name + "_{}_base_build".format(base_trim_str),
                outs = [],
                trim_nonlisted_kmi = device_trim,
                kmi_symbol_list = "symbol_list_device",
                tags = ["manual"],
            )

            _fake_extracted_symbols(
                name = name + "_{}_{}_device_extracted_symbols".format(base_trim_str, device_trim_str),
                kernel_build = name + "_{}_{}_device_build".format(base_trim_str, device_trim_str),
            )

            _notrim_transition_analysis_test(
                name = name + "_{}_{}_device_test".format(base_trim_str, device_trim_str),
                target_under_test = name + "_{}_{}_device_extracted_symbols".format(base_trim_str, device_trim_str),
            )
            tests.append(name + "_{}_{}_device_test".format(base_trim_str, device_trim_str))

    native.test_suite(
        name = name,
        tests = tests,
    )
