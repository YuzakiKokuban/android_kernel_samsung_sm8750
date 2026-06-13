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

"""Impl of `embedded_exec`."""

load("@bazel_skylib//lib:shell.bzl", "shell")
load(":exec_aspect.bzl", "ExecAspectInfo", "exec_aspect")

visibility([
    "//build/bazel_common_rules/exec/...",
    "//build/bazel_common_rules/dist/...",
])

def _impl(ctx):
    target = ctx.attr.actual
    files_to_run = target[DefaultInfo].files_to_run
    if not files_to_run or not files_to_run.executable:
        fail("{}: {} is not an executable".format(ctx.label, target))

    out_file = ctx.actions.declare_file(ctx.label.name)

    content = "#!{}\n".format(ctx.attr.hashbang)

    expand_location_targets = []
    for dependant_attr in ("data", "srcs", "deps"):
        dependants = getattr(target[ExecAspectInfo], dependant_attr)
        if dependants:
            expand_location_targets += dependants

    args = target[ExecAspectInfo].args
    if not args:
        args = []
    quoted_args = " ".join([shell.quote(ctx.expand_location(arg, expand_location_targets)) for arg in args])

    env = target[ExecAspectInfo].env
    if not env:
        env = {}

    quoted_env = " ".join(["{}={}".format(k, shell.quote(ctx.expand_location(v, expand_location_targets))) for k, v in env.items()])

    content += '{} {} {} "$@"'.format(quoted_env, target[DefaultInfo].files_to_run.executable.short_path, quoted_args)

    ctx.actions.write(out_file, content, is_executable = True)

    runfiles = ctx.runfiles(files = ctx.files.actual)
    runfiles = runfiles.merge_all([target[DefaultInfo].default_runfiles])

    return DefaultInfo(
        files = depset([out_file]),
        executable = out_file,
        runfiles = runfiles,
    )

embedded_exec = rule(
    implementation = _impl,
    attrs = {
        "actual": attr.label(doc = "The actual executable.", aspects = [exec_aspect]),
        "hashbang": attr.string(doc = "The hashbang of the script", default = "/bin/bash -e"),
    },
    executable = True,
)
