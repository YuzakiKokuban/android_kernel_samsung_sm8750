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

"""Helper functions for `kernel_env` etc. to resolve toolchains"""

load(
    ":common_providers.bzl",
    "KernelEnvToolchainsInfo",
)

visibility("//build/kernel/kleaf/impl/...")

def _toolchains_transition_impl(_settings, attr):
    return {
        "//command_line_option:platforms": str(attr.target_platform),
        "//command_line_option:host_platform": str(attr.exec_platform),
    }

_toolchains_transition = transition(
    implementation = _toolchains_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:platforms",
        "//command_line_option:host_platform",
    ],
)

def _attrs():
    return {
        "_toolchains": attr.label(
            doc = "Provides all toolchains that the kernel build needs.",
            default = "//build/kernel/kleaf/impl:kernel_toolchains",
            providers = [KernelEnvToolchainsInfo],
            cfg = _toolchains_transition,
        ),
        "target_platform": attr.label(
            mandatory = True,
            doc = """Target platform that describes characteristics of the target device.

                See https://bazel.build/extending/platforms.
            """,
        ),
        "exec_platform": attr.label(
            mandatory = True,
            doc = """Execution platform, where the build is executed.

                See https://bazel.build/extending/platforms.
            """,
        ),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    }

def _get_toolchains(ctx):
    return ctx.attr._toolchains[0][KernelEnvToolchainsInfo]

kernel_toolchains_utils = struct(
    attrs = _attrs,
    get = _get_toolchains,
)
