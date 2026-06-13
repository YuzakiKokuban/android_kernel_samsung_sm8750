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

"""Builds vmlinux.btf."""

load(":common_providers.bzl", "KernelEnvInfo")
load(":debug.bzl", "debug")

visibility("//build/kernel/kleaf/...")

def _btf_impl(ctx):
    inputs = [
        ctx.file.vmlinux,
    ]
    transitive_inputs = [ctx.attr.env[KernelEnvInfo].inputs]
    tools = ctx.attr.env[KernelEnvInfo].tools
    out_file = ctx.actions.declare_file("{}/vmlinux.btf".format(ctx.label.name))
    out_dir = out_file.dirname

    # We need KernelEnvInfo for llvm-strip.
    # TODO(b/272164611): We can get it from the clang toolchain; then we can use
    #  hermetic tools.
    command = ctx.attr.env[KernelEnvInfo].setup + """
              mkdir -p {out_dir}
              cp -Lp {vmlinux} {btf}
              pahole -J {btf}
              llvm-strip --strip-debug {btf}
    """.format(
        vmlinux = ctx.file.vmlinux.path,
        btf = out_file.path,
        out_dir = out_dir,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "Btf",
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = [out_file],
        tools = tools,
        progress_message = "Building vmlinux.btf {}".format(ctx.label),
        command = command,
    )
    return DefaultInfo(files = depset([out_file]))

btf = rule(
    implementation = _btf_impl,
    doc = "Build vmlinux.btf",
    attrs = {
        "vmlinux": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "env": attr.label(
            mandatory = True,
            providers = [KernelEnvInfo],
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
)
