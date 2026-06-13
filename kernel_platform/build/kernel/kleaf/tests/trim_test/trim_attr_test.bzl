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

"""Tests `trim_nonlisted_kmi`."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load(
    ":trim_aspect.bzl",
    "TrimAspectInfo",
    "check_kernel_build_trim_attr",
    "trim_aspect",
)

def _trim_analysis_test_impl(ctx):
    env = analysistest.begin(ctx)

    target_under_test = analysistest.target_under_test(env)

    if target_under_test[TrimAspectInfo].base_info == None:
        asserts.false(
            env,
            ctx.attr.has_base,
            target_under_test.label,
        )
    else:
        check_kernel_build_trim_attr(
            env,
            ctx.attr.base_expect_trim,
            target_under_test[TrimAspectInfo].base_info,
        )
    check_kernel_build_trim_attr(
        env,
        ctx.attr.expect_trim,
        target_under_test[TrimAspectInfo],
    )

    return analysistest.end(env)

_trim_analysis_test = analysistest.make(
    impl = _trim_analysis_test_impl,
    attrs = {
        "has_base": attr.bool(mandatory = True),
        "base_expect_trim": attr.bool(),
        "expect_trim": attr.bool(mandatory = True),
    },
    extra_target_under_test_aspects = [trim_aspect],
)

def trim_attr_test(name):
    """Tests the effect of `trim_nonlisted_kmi` on dependencies.

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

        _trim_analysis_test(
            name = name + "_{}_base_test".format(base_trim_str),
            target_under_test = name + "_{}_base_build".format(base_trim_str),
            # {name}_{base_trim_str}_base_build doens't have a base_kernel
            has_base = False,
            expect_trim = base_trim,
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

            _trim_analysis_test(
                name = name + "_{}_{}_device_test".format(base_trim_str, device_trim_str),
                target_under_test = name + "_{}_{}_device_build".format(base_trim_str, device_trim_str),
                has_base = True,
                base_expect_trim = base_trim,
                expect_trim = device_trim,
            )
            tests.append(name + "_{}_{}_device_test".format(base_trim_str, device_trim_str))

    native.test_suite(
        name = name,
        tests = tests,
    )
