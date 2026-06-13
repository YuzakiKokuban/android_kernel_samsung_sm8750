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

"""Merge `kernel-uapi-headers.tar.gz`."""

load("//build/kernel/kleaf:directory_with_structure.bzl", dws = "directory_with_structure")
load(
    ":common_providers.bzl",
    "KernelBuildUapiInfo",
    "KernelModuleInfo",
)
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _merged_kernel_uapi_headers_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)

    kernel_build = ctx.attr.kernel_build

    # srcs and dws_srcs are the list of sources to merge.
    # Early elements = higher priority. srcs has higher priority than dws_srcs.
    srcs = kernel_build[KernelBuildUapiInfo].kernel_uapi_headers.to_list()

    # TODO(b/256688440): Avoid depset[directory_with_structure] to_list
    dws_srcs = depset(transitive = [kernel_module[KernelModuleInfo].kernel_uapi_headers_dws_depset for kernel_module in ctx.attr.kernel_modules]).to_list()

    inputs = [] + srcs

    for dws_src in dws_srcs:
        inputs += dws.files(dws_src)

    out_file = ctx.actions.declare_file("{}/kernel-uapi-headers.tar.gz".format(ctx.attr.name))
    intermediates_dir = utils.intermediates_dir(ctx)

    command = hermetic_tools.setup
    command += """
        mkdir -p {intermediates_dir}
    """.format(
        intermediates_dir = intermediates_dir,
    )

    # Extract the source tarballs in low to high priority order.
    for dws_src in reversed(dws_srcs):
        # Copy the directory over, overwriting existing files. Add write permission
        # targets with higher priority can overwrite existing files.
        command += dws.restore(
            dws_src,
            dst = intermediates_dir,
            options = "-aL --chmod=+w",
        )

    for src in reversed(srcs):
        command += """
            tar xf {src} -C {intermediates_dir}
        """.format(
            src = src.path,
            intermediates_dir = intermediates_dir,
        )

    command += """
        tar czf {out_file} -C {intermediates_dir} usr/
        rm -rf {intermediates_dir}
    """.format(
        out_file = out_file.path,
        intermediates_dir = intermediates_dir,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [out_file],
        tools = hermetic_tools.deps,
        progress_message = "Merging kernel-uapi-headers.tar.gz {}".format(ctx.label),
        command = command,
        mnemonic = "MergedKernelUapiHeaders",
    )
    return DefaultInfo(files = depset([out_file]))

merged_kernel_uapi_headers = rule(
    implementation = _merged_kernel_uapi_headers_impl,
    doc = """Merge `kernel-uapi-headers.tar.gz`.

On certain devices, kernel modules install additional UAPI headers. Use this
rule to add these module UAPI headers to the final `kernel-uapi-headers.tar.gz`.

If there are conflicts of file names in the source tarballs, files higher in
the list have higher priority:
1. UAPI headers from the `base_kernel` of the `kernel_build` (ususally the GKI build)
2. UAPI headers from the `kernel_build` (usually the device build)
3. UAPI headers from ``kernel_modules`. Order among the modules are undetermined.
""",
    attrs = {
        "kernel_build": attr.label(
            doc = "The `kernel_build`",
            mandatory = True,
            providers = [KernelBuildUapiInfo],
        ),
        "kernel_modules": attr.label_list(
            doc = """A list of external `kernel_module`s to merge `kernel-uapi-headers.tar.gz`""",
            providers = [KernelModuleInfo],
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
    toolchains = [hermetic_toolchain.type],
)
