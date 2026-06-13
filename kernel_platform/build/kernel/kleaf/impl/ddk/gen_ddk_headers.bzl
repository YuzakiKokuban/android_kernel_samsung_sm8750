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

"""Generates the list of DDK headers by analyzing inputs to modules."""

load("@bazel_skylib//lib:shell.bzl", "shell")
load(":ddk/analyze_inputs.bzl", "analyze_inputs")

visibility("//build/kernel/kleaf/...")

def _gen_ddk_headers_impl(ctx):
    executable = ctx.actions.declare_file(ctx.label.name + ".sh")

    content = """#!/bin/bash -e
                 {generator} --input {src} $@
                 """.format(
        src = shell.quote(ctx.file.src.short_path),
        generator = shell.quote(ctx.executable._generator.short_path),
    )
    ctx.actions.write(executable, content, is_executable = True)

    runfiles = ctx.runfiles(files = [ctx.file.src])
    transitive_runfiles = [
        ctx.attr._generator[DefaultInfo].default_runfiles,
    ]
    runfiles = runfiles.merge_all(transitive_runfiles)

    return DefaultInfo(
        files = depset([executable]),
        executable = executable,
        runfiles = runfiles,
    )

_gen_ddk_headers = rule(
    implementation = _gen_ddk_headers_impl,
    attrs = {
        "src": attr.label(allow_single_file = True),
        "_generator": attr.label(
            default = "//build/kernel/kleaf/impl:ddk/gen_ddk_headers",
            executable = True,
            cfg = "exec",
        ),
    },
    executable = True,
)

def gen_ddk_headers(
        name,
        target,
        gen_files_archives = None):
    analyze_inputs(
        name = name + "_inputs",
        exclude_filters = [
            "arch/arm64/include/generated/*",
            "arch/x86/include/generated/*",
            "include/generated/*",
        ],
        include_filters = ["*.h"],
        gen_files_archives = gen_files_archives,
        deps = [target],
    )

    _gen_ddk_headers(
        name = name,
        src = name + "_inputs",
    )
