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

"""Extracts symbols from kernel binaries."""

load(":abi/abi_transitions.bzl", "abi_common_attrs", "notrim_transition")
load(
    ":common_providers.bzl",
    "KernelBuildAbiInfo",
    "KernelModuleInfo",
    "KernelSerializedEnvInfo",
)
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _extracted_symbols_impl(ctx):
    if ctx.attr.kernel_build[KernelBuildAbiInfo].trim_nonlisted_kmi:
        fail("{}: Requires `kernel_build` {} to have `trim_nonlisted_kmi = False`.".format(
            ctx.label,
            ctx.attr.kernel_build.label,
        ))

    if ctx.attr.kmi_symbol_list_add_only and not ctx.file.src:
        fail("{}: kmi_symbol_list_add_only requires kmi_symbol_list.".format(ctx.label))

    out = ctx.actions.declare_file("{}/extracted_symbols".format(ctx.attr.name))
    intermediates_dir = utils.intermediates_dir(ctx)

    vmlinux = utils.find_file(name = "vmlinux", files = ctx.files.kernel_build, what = "{}: kernel_build".format(ctx.attr.name), required = True)
    in_tree_modules = utils.find_files(suffix = ".ko", files = ctx.files.kernel_build)
    srcs = [
        vmlinux,
    ]
    srcs += in_tree_modules

    # external modules
    for kernel_module in ctx.attr.kernel_modules:
        if KernelModuleInfo in kernel_module:
            srcs += kernel_module[KernelModuleInfo].files.to_list()
        else:
            srcs += kernel_module.files.to_list()

    inputs = [] + srcs
    transitive_inputs = [ctx.attr.kernel_build[KernelSerializedEnvInfo].inputs]
    tools = [ctx.executable._extract_symbols]
    transitive_tools = [ctx.attr.kernel_build[KernelSerializedEnvInfo].tools]

    cp_src_cmd = ""
    flags = ["--symbol-list", out.path]
    if not ctx.attr.module_grouping:
        flags.append("--skip-module-grouping")
    if ctx.attr.kmi_symbol_list_add_only:
        flags.append("--additions-only")
        inputs.append(ctx.file.src)

        # Follow symlinks because we are in the execroot.
        # Do not preserve permissions because we are overwriting the file immediately.
        cp_src_cmd = "cp -L {src} {out}".format(
            src = ctx.file.src.path,
            out = out.path,
        )

    for base_name_module in ctx.attr.kernel_modules_exclude_list:
        flags.append("--module-exclude={}".format(base_name_module))

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
        # Extract archive and copy the GKI modules First
        # TODO(/b/243570975): Use tar wildcards & xform once prebuilt supports it, as below:
        # tar --directory={intermediates_dir} --wildcards --xform='s#^.+/##x' -xf {base_modules_archive} '*.ko
        mkdir -p {intermediates_dir}/temp
        tar xf {base_modules_archive} -C {intermediates_dir}/temp
        find {intermediates_dir}/temp -name '*.ko' -exec mv -t {intermediates_dir} {{}} \\;
        rm -rf {intermediates_dir}/temp
        # Copy other inputs including vendor modules; this will overwrite modules being overridden
        cp -pfl {srcs} {intermediates_dir}
        {cp_src_cmd}
        {extract_symbols} {flags} {intermediates_dir}
        rm -rf {intermediates_dir}
    """.format(
        srcs = " ".join([file.path for file in srcs]),
        intermediates_dir = intermediates_dir,
        extract_symbols = ctx.executable._extract_symbols.path,
        flags = " ".join(flags),
        cp_src_cmd = cp_src_cmd,
        base_modules_archive = base_modules_archive.path,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = [out],
        command = command,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Extracting symbols {}".format(ctx.label),
        mnemonic = "KernelExtractedSymbols",
    )

    return DefaultInfo(files = depset([out]))

extracted_symbols = rule(
    implementation = _extracted_symbols_impl,
    attrs = {
        # We can't use kernel_filegroup + hermetic_tools here because
        # - extract_symbols depends on the clang toolchain, which requires us to
        #   know the toolchain_version ahead of time.
        # - We also don't have the necessity to extract symbols from prebuilts.
        "kernel_build": attr.label(providers = [KernelSerializedEnvInfo, KernelBuildAbiInfo]),
        # KernelModuleInfo
        "kernel_modules": attr.label_list(allow_files = True),
        "kernel_modules_exclude_list": attr.string_list(
            doc = "Base name list of kernel modules to exclude from.",
        ),
        "module_grouping": attr.bool(default = True),
        "src": attr.label(doc = "Source `abi_gki_*` file. Used when `kmi_symbol_list_add_only`.", allow_single_file = True),
        "kmi_symbol_list_add_only": attr.bool(),
        "_extract_symbols": attr.label(
            default = "//build/kernel:extract_symbols",
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
