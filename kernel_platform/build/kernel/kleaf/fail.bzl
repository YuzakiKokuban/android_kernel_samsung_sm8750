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
A rule that fails.
"""

load("@bazel_skylib//lib:shell.bzl", "shell")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _impl(ctx):
    fail(ctx.attr.message if ctx.attr.message else "unknown error")

fail_rule = rule(
    implementation = _impl,
    doc = "A rule that fails at analysis phase",
    attrs = {
        "message": attr.string(doc = "fail message"),
    },
)

def _fail_action_impl(ctx):
    if ctx.attr._write_to_file[BuildSettingInfo].value:
        file = ctx.actions.declare_file(ctx.attr.name + "/expected_message.txt")
        ctx.actions.write(output = file, content = ctx.attr.message)
    else:
        file = ctx.actions.declare_file(ctx.attr.name)
        ctx.actions.run_shell(
            inputs = [],
            outputs = [file],
            command = """
                # Ensure hermeticity; do not rely on original PATH. We don't need
                # a full hermetic toolchain here because only builtins are used.
                PATH=

                echo {quoted_message} >&2
                exit 1
            """.format(
                quoted_message = shell.quote(ctx.attr.message),
            ),
            mnemonic = "FailAction",
        )

    return DefaultInfo(files = depset([file]))

fail_action = rule(
    implementation = _fail_action_impl,
    doc = "A rule that fails at execution phase",
    attrs = {
        "message": attr.string(doc = "fail message"),
        "_write_to_file": attr.label(default = "//build/kernel/kleaf/impl:fail_action_write_to_file"),
    },
)
