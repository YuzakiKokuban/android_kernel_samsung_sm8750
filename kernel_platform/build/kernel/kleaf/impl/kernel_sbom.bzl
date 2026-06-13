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

"""Generate an SPDX SBOM."""

load("@kernel_toolchain_info//:dict.bzl", "VARS")
load(":common_providers.bzl", "KernelBuildUnameInfo")

visibility("//build/kernel/kleaf/...")

def _kernel_sbom_impl(ctx):
    output_file = ctx.actions.declare_file("{}/kernel_sbom.spdx.json".format(ctx.label.name))
    kernel_release = ctx.attr.kernel_build[KernelBuildUnameInfo].kernel_release

    srcs_depset = depset(transitive = [target.files for target in ctx.attr.srcs])

    args = ctx.actions.args()
    args.add("--output_file", output_file)
    args.add_all("--files", srcs_depset)
    args.add("--version_file", kernel_release)
    args.add("--readelf", ctx.executable._readelf)

    ctx.actions.run(
        mnemonic = "KernelSbom",
        inputs = depset([kernel_release, ctx.executable._readelf], transitive = [srcs_depset]),
        outputs = [output_file],
        executable = ctx.executable._kernel_sbom,
        arguments = [args],
        progress_message = "Generating Kernel SBOM {}".format(ctx.label),
    )

    return [
        DefaultInfo(files = depset([output_file])),
    ]

kernel_sbom = rule(
    implementation = _kernel_sbom_impl,
    doc = """Generate an SPDX SBOM for kernels.""",
    attrs = {
        "srcs": attr.label_list(mandatory = True, allow_files = True),
        "kernel_build": attr.label(
            mandatory = True,
            providers = [KernelBuildUnameInfo],
        ),
        "_kernel_sbom": attr.label(
            default = "//build/kernel/kleaf:kernel_sbom",
            cfg = "exec",
            executable = True,
        ),
        "_readelf": attr.label(
            default =
                "//prebuilts/clang/host/linux-x86/clang-{}:bin/llvm-readelf".format(VARS["CLANG_VERSION"]),
            cfg = "exec",
            executable = True,
            allow_files = True,
        ),
    },
)
