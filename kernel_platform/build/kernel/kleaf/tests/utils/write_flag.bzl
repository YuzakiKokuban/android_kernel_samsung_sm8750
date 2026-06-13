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

"""A rule that writes the value of a flag."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(
        output = output,
        content = str(ctx.attr.flag[BuildSettingInfo].value),
    )
    return DefaultInfo(files = depset([output]))

write_flag = rule(
    doc = "Write the value of a flag to a file.",
    implementation = _impl,
    attrs = {
        "flag": attr.label(doc = "The flag.", mandatory = True),
    },
)
