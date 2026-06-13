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

"""Compress the unstripped modules into a tarball."""

load(
    ":common_providers.bzl",
    "KernelUnstrippedModulesInfo",
)
load(
    ":constants.bzl",
    "UNSTRIPPED_MODULES_ARCHIVE",
)
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _kernel_unstripped_modules_archive_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)

    # Early elements = higher priority. In-tree modules from base_kernel has highest priority,
    # then in-tree modules of the device kernel_build, then external modules (in an undetermined
    # order).
    directories_depsets = [ctx.attr.kernel_build[KernelUnstrippedModulesInfo].directories]
    directories_depsets += [kernel_module[KernelUnstrippedModulesInfo].directories for kernel_module in ctx.attr.kernel_modules]
    srcs = depset(transitive = directories_depsets, order = "postorder").to_list()

    out_file = ctx.actions.declare_file("{}/{}".format(ctx.attr.name, UNSTRIPPED_MODULES_ARCHIVE))
    unstripped_dir = ctx.genfiles_dir.path + "/unstripped"

    command = hermetic_tools.setup
    command += """
        mkdir -p {unstripped_dir}
    """.format(unstripped_dir = unstripped_dir)

    # Copy the source ko files in low to high priority order.
    for src in reversed(srcs):
        # src could be empty, so use find + cp
        # TODO(https://github.com/landley/toybox/issues/431): Using `-L` instead
        #  of `-l` to force copy the files intead of their symlinks due to an
        #  issue with toybox tar which is not following them when using `--sort`.
        command += """
            find {src} -name '*.ko' -exec cp -f -L -t {unstripped_dir} {{}} +
        """.format(
            src = src.path,
            unstripped_dir = unstripped_dir,
        )

    command += """
        tar -czhf {out_file} -C $(dirname {unstripped_dir}) $(basename {unstripped_dir})
    """.format(
        out_file = out_file.path,
        unstripped_dir = unstripped_dir,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = srcs,
        outputs = [out_file],
        tools = hermetic_tools.deps,
        progress_message = "Compressing unstripped modules {}".format(ctx.label),
        command = command,
        mnemonic = "KernelUnstrippedModulesArchive",
    )
    return DefaultInfo(files = depset([out_file]))

kernel_unstripped_modules_archive = rule(
    implementation = _kernel_unstripped_modules_archive_impl,
    doc = """Compress the unstripped modules into a tarball.

Add this target to a `copy_to_dist_dir` rule to copy it to the distribution
directory, or `DIST_DIR`.
""",
    attrs = {
        "kernel_build": attr.label(
            doc = """A [`kernel_build`](#kernel_build) to retrieve unstripped in-tree modules from.

It requires `collect_unstripped_modules = True`. If the `kernel_build` has a `base_kernel`, the rule
also retrieves unstripped in-tree modules from the `base_kernel`, and requires the
`base_kernel` has `collect_unstripped_modules = True`.
""",
            providers = [KernelUnstrippedModulesInfo],
        ),
        "kernel_modules": attr.label_list(
            doc = """A list of external [`kernel_module`](#kernel_module)s to retrieve unstripped external modules from.

It requires that the base `kernel_build` has `collect_unstripped_modules = True`.
""",
            providers = [KernelUnstrippedModulesInfo],
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
    toolchains = [hermetic_toolchain.type],
)
