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

"""Rules for ABI extraction."""

load(":abi/abi_transitions.bzl", "abi_common_attrs", "with_vmlinux_transition")
load(
    ":common_providers.bzl",
    "KernelBuildAbiInfo",
    "KernelModuleInfo",
    "KernelUnstrippedModulesInfo",
)
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _abi_dump_impl(ctx):
    kernel_utils.check_kernel_build(
        [target[KernelModuleInfo] for target in ctx.attr.kernel_modules],
        ctx.attr.kernel_build.label,
        ctx.label,
    )

    # Run both methods until STG is fully adopted.
    full_abi_out_file_stg = _abi_dump_full_stg(ctx)
    abi_out_file_stg = _abi_dump_filtered_stg(ctx, full_abi_out_file_stg)

    return [
        DefaultInfo(files = depset([
            full_abi_out_file_stg,
            abi_out_file_stg,
        ])),
        OutputGroupInfo(
            abi_out_file = depset([abi_out_file_stg]),
        ),
    ]

def _unstripped_dirs(ctx):
    unstripped_dirs = []

    unstripped_dir_provider_targets = [ctx.attr.kernel_build] + ctx.attr.kernel_modules
    unstripped_dir_providers = [target[KernelUnstrippedModulesInfo] for target in unstripped_dir_provider_targets]
    for prov, target in zip(unstripped_dir_providers, unstripped_dir_provider_targets):
        dirs_for_target = prov.directories.to_list()
        if not dirs_for_target:
            fail("{}: Requires dep {} to set collect_unstripped_modules = True".format(
                ctx.label,
                target.label,
            ))
        unstripped_dirs += dirs_for_target

    return unstripped_dirs

def _find_vmlinux(ctx):
    return utils.find_file(
        name = "vmlinux",
        files = ctx.files.kernel_build,
        what = "{}: kernel_build".format(ctx.attr.name),
        required = True,
    )

def _abi_dump_full_stg(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    full_abi_out_file = ctx.actions.declare_file("{}/abi-full.stg".format(ctx.attr.name))
    vmlinux = _find_vmlinux(ctx)
    unstripped_dirs = _unstripped_dirs(ctx)

    inputs = [vmlinux, ctx.file._stg]
    inputs += unstripped_dirs

    # Collect all modules from all directories
    all_modules = ""
    for unstripped_dir in unstripped_dirs:
        all_modules += "$(find {dir_path} -name '*.ko') ".format(
            dir_path = unstripped_dir.path,
        )

    command = hermetic_tools.setup + """
        {stg} --files '*.h' --output {full_abi_out_file} --elf {vmlinux} {all_modules}
    """.format(
        stg = ctx.file._stg.path,
        full_abi_out_file = full_abi_out_file.path,
        vmlinux = vmlinux.path,
        all_modules = all_modules,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [full_abi_out_file],
        tools = hermetic_tools.deps,
        command = command,
        mnemonic = "AbiDumpFullStg",
        progress_message = "[stg] Extracting ABI {}".format(ctx.label),
    )
    return full_abi_out_file

def _abi_dump_filtered_stg(ctx, full_abi_out_file):
    hermetic_tools = hermetic_toolchain.get(ctx)
    abi_out_file = ctx.actions.declare_file("{}/abi.stg".format(ctx.attr.name))
    combined_abi_symbollist = ctx.attr.kernel_build[KernelBuildAbiInfo].combined_abi_symbollist
    inputs = [full_abi_out_file]
    command = hermetic_tools.setup

    if combined_abi_symbollist:
        inputs += [
            ctx.file._stg,
            combined_abi_symbollist,
        ]

        command += """
            {stg} --symbols :{abi_symbollist} --output {abi_out_file} --stg {full_abi_out_file}
        """.format(
            stg = ctx.file._stg.path,
            abi_symbollist = combined_abi_symbollist.path,
            abi_out_file = abi_out_file.path,
            full_abi_out_file = full_abi_out_file.path,
        )
    else:
        command += """
            cp -p {full_abi_out_file} {abi_out_file}
        """.format(
            abi_out_file = abi_out_file.path,
            full_abi_out_file = full_abi_out_file.path,
        )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [abi_out_file],
        tools = hermetic_tools.deps,
        command = command,
        mnemonic = "AbiDumpFilteredStg",
        progress_message = "[stg] Filtering ABI dump {}".format(ctx.label),
    )
    return abi_out_file

abi_dump = rule(
    implementation = _abi_dump_impl,
    doc = "Extracts the ABI.",
    attrs = {
        "kernel_build": attr.label(providers = [KernelBuildAbiInfo, KernelUnstrippedModulesInfo]),
        "kernel_modules": attr.label_list(providers = [KernelUnstrippedModulesInfo]),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
        "_stg": attr.label(
            default = "//prebuilts/kernel-build-tools:linux-x86/bin/stg",
            allow_single_file = True,
            cfg = "exec",
            executable = True,
        ),
    } | abi_common_attrs(),
    cfg = with_vmlinux_transition,
    toolchains = [hermetic_toolchain.type],
)
