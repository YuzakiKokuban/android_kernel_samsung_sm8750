# Copyright 2023 The Bazel Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Like filegroup, but applies transitions to Android."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

visibility("//build/kernel/kleaf/...")

def _android_filegroup_transition_impl(_settings, attr):
    return {
        "//command_line_option:platforms": "//build/kernel/kleaf/impl:android_{}".format(attr.cpu),
    }

_android_filegroup_transition = transition(
    implementation = _android_filegroup_transition_impl,
    inputs = [],
    outputs = ["//command_line_option:platforms"],
)

def _android_filegroup_impl(ctx):
    if not ctx.attr._config_is_hermetic_cc[BuildSettingInfo].value:
        fail("{}: android_filegroup must be built with --config=hermetic_cc".format(ctx.label))

    return DefaultInfo(files = depset(transitive = [target.files for target in ctx.attr.srcs]))

android_filegroup = rule(
    implementation = _android_filegroup_impl,
    doc = """Like filegroup, but applies transitions to Android.""",
    attrs = {
        "srcs": attr.label_list(doc = "Sources of the filegroup."),
        "cpu": attr.string(
            doc = "Architecture.",
            default = "arm64",
            values = [
                "arm",
                "arm64",
                "i386",
                "riscv64",
                "x86_64",
            ],
        ),
        "_config_is_hermetic_cc": attr.label(default = "//build/kernel/kleaf:config_hermetic_cc"),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    cfg = _android_filegroup_transition,
)
