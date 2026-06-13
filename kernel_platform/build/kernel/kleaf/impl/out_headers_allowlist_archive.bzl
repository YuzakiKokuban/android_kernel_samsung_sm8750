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

"""An archive of headers in certain subdirectories under `OUT_DIR`."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:shell.bzl", "shell")
load(
    ":common_providers.bzl",
    "KernelBuildInfo",
)
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _out_headers_allowlist_archive_impl(ctx):
    out_file = ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))
    out_dir = paths.join(utils.intermediates_dir(ctx), "out_dir")

    subdirs_pattern = "^" + ("|".join([subdir + "/" for subdir in ctx.attr.subdirs]))

    hermetic_tools = hermetic_toolchain.get(ctx)

    command = hermetic_tools.setup + """
            # Restore headers in OUT_DIR
              mkdir -p {out_dir}
              tar tf {out_dir_kernel_headers_tar} | \\
                grep -E {subdirs_pattern} | \\
                tar xf {out_dir_kernel_headers_tar} -C {out_dir} -T -
            # Create archive
              tar czf {out_file} -C {out_dir} --transform 's:^./::' .
    """.format(
        out_dir = out_dir,
        out_file = out_file.path,
        out_dir_kernel_headers_tar = ctx.attr.kernel_build[KernelBuildInfo].out_dir_kernel_headers_tar.path,
        subdirs_pattern = shell.quote(subdirs_pattern),
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "OutHeadersAllowlistArchive",
        inputs = [ctx.attr.kernel_build[KernelBuildInfo].out_dir_kernel_headers_tar],
        outputs = [out_file],
        tools = hermetic_tools.deps,
        progress_message = "Creating headers archive {}".format(ctx.label),
        command = command,
    )

    return [
        DefaultInfo(files = depset([out_file])),
    ]

out_headers_allowlist_archive = rule(
    implementation = _out_headers_allowlist_archive_impl,
    doc = "An archive of headers in certain subdirectories under `OUT_DIR`.",
    attrs = {
        "kernel_build": attr.label(
            mandatory = True,
            providers = [KernelBuildInfo],  # for out_dir_kernel_headers_tar only
        ),
        "subdirs": attr.string_list(
            doc = "A list of subdirectories under `OUT_DIR` to find headers from",
            mandatory = True,
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
    toolchains = [hermetic_toolchain.type],
)
