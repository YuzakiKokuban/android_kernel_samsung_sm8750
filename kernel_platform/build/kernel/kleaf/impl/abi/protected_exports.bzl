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

"""Extracts protected exports from protected kernel modules."""

load(":abi/abi_transitions.bzl", "abi_common_attrs", "notrim_transition")
load(
    ":common_providers.bzl",
    "KernelBuildAbiInfo",
    "KernelSerializedEnvInfo",
)
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _protected_exports_impl(ctx):
    if ctx.attr.kernel_build[KernelBuildAbiInfo].trim_nonlisted_kmi:
        fail("{}: Requires `kernel_build` {} to have `trim_nonlisted_kmi = False`.".format(
            ctx.label,
            ctx.attr.kernel_build.label,
        ))

    if not ctx.file.protected_modules_list_file:
        fail("{}: {} does not produce any files.".format(ctx.label, ctx.file.protected_modules_list_file))

    out = ctx.actions.declare_file("{}/protected_exports".format(ctx.attr.name))
    intermediates_dir = utils.intermediates_dir(ctx)

    inputs = [
        ctx.file.protected_modules_list_file,
    ]
    transitive_inputs = [ctx.attr.kernel_build[KernelSerializedEnvInfo].inputs]
    tools = [ctx.executable._extract_protected_exports]
    transitive_tools = [ctx.attr.kernel_build[KernelSerializedEnvInfo].tools]

    flags = ["--protected-exports-list", out.path]
    flags += ["--gki-protected-modules-list", ctx.file.protected_modules_list_file.path]

    # Get the signed and stripped module archive for the GKI modules
    base_modules_archive = ctx.attr.kernel_build[KernelBuildAbiInfo].base_modules_staging_archive
    if not base_modules_archive:
        base_modules_archive = ctx.attr.kernel_build[KernelBuildAbiInfo].modules_staging_archive
    inputs.append(base_modules_archive)

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.kernel_build[KernelSerializedEnvInfo],
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )

    command += """
        mkdir -p {intermediates_dir}
        mkdir -p {intermediates_dir}/temp
        # Archive layout is lib/modules/<kernel_version>/kernel/<modules tree>
        # Only extract kernel and below to get rid of variable <kernel version>
        tar xf {base_modules_archive} --strip-components=4 -C {intermediates_dir}/temp
        # We only need to match the <module tree> under kernel against protected modules list
        mv {intermediates_dir}/temp/kernel/* {intermediates_dir}/
        rm -rf {intermediates_dir}/temp
        {extract_protected_exports} {flags} {intermediates_dir}
        rm -rf {intermediates_dir}
    """.format(
        intermediates_dir = intermediates_dir,
        extract_protected_exports = ctx.executable._extract_protected_exports.path,
        flags = " ".join(flags),
        base_modules_archive = base_modules_archive.path,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = [out],
        command = command,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Extracting protected exports {}".format(ctx.label),
        mnemonic = "KernelProtectedExports",
    )

    return DefaultInfo(files = depset([out]))

protected_exports = rule(
    implementation = _protected_exports_impl,
    attrs = {
        # We can't use kernel_filegroup + hermetic_tools here because
        # - extract_protected_exports depends on the clang toolchain, which requires us to
        #   know the toolchain_version ahead of time.
        # - We also don't have the necessity to extract symbols from prebuilts.
        "kernel_build": attr.label(providers = [KernelSerializedEnvInfo, KernelBuildAbiInfo]),
        "protected_modules_list_file": attr.label(doc = "List of protected modules whose exports needs to be extracted.", allow_single_file = True),
        "_extract_protected_exports": attr.label(
            default = "//build/kernel:extract_protected_exports",
            cfg = "exec",
            executable = True,
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    } | abi_common_attrs(),
    cfg = notrim_transition,
)
