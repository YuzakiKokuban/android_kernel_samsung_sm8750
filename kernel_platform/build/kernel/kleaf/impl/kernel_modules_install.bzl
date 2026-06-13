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
"""
A rule that runs depmod in the module installation directory.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("//build/kernel/kleaf:directory_with_structure.bzl", dws = "directory_with_structure")
load(
    ":common_providers.bzl",
    "GcovInfo",
    "KernelBuildExtModuleInfo",
    "KernelBuildInfo",
    "KernelCmdsInfo",
    "KernelImagesInfo",
    "KernelModuleInfo",
    "KernelSerializedEnvInfo",
)
load(":debug.bzl", "debug")
load(":gcov_utils.bzl", "gcov_attrs", "get_merge_gcno_step")
load(
    ":utils.bzl",
    "kernel_utils",
    "utils",
)

visibility("//build/kernel/kleaf/...")

# To avoid being abused, output is limited to modules.* files
_OUT_ALLOWLIST = [
    "modules.dep",
    "modules.alias",
    "modules.builtin",
    "modules.symbols",
    "modules.softdep",
]

def _kernel_modules_install_impl(ctx):
    kernel_build_infos = None
    if ctx.attr.kernel_build:
        kernel_build_infos = kernel_utils.create_kernel_module_kernel_build_info(ctx.attr.kernel_build)
    elif ctx.attr.kernel_modules:
        kernel_build_infos = ctx.attr.kernel_modules[0][KernelModuleInfo].kernel_build_infos

    if not kernel_build_infos:
        fail("No `kernel_build` or `kernel_modules` provided.")

    kernel_utils.check_kernel_build(
        [target[KernelModuleInfo] for target in ctx.attr.kernel_modules],
        kernel_build_infos.label,
        ctx.label,
    )

    # A list of declared files for outputs of kernel_module rules
    external_modules = []

    # A list of additional files other than kernel modules.
    outs = []

    # TODO(b/256688440): Avoid depset[directory_with_structure] to_list
    modules_staging_dws_depset = depset(transitive = [
        kernel_module[KernelModuleInfo].modules_staging_dws_depset
        for kernel_module in ctx.attr.kernel_modules
    ])
    modules_staging_dws_list = modules_staging_dws_depset.to_list()

    inputs = []
    inputs.append(
        kernel_build_infos.ext_module_info.modules_staging_archive,
    )

    for input_modules_staging_dws in modules_staging_dws_list:
        inputs += dws.files(input_modules_staging_dws)

    module_files = depset(transitive = [
        kernel_module[KernelModuleInfo].files
        for kernel_module in ctx.attr.kernel_modules
    ]).to_list()
    for module_file in module_files:
        declared_file = ctx.actions.declare_file("{}/{}".format(ctx.label.name, module_file.basename))
        external_modules.append(declared_file)

    for out in ctx.attr.outs:
        if out not in _OUT_ALLOWLIST:
            fail(
                """{}: {} is not allowed in outs.
                Please refer to the list of allowed files {}""".format(
                    ctx.label,
                    out,
                    _OUT_ALLOWLIST,
                ),
            )
        out_file = ctx.actions.declare_file("{}/{}".format(ctx.label.name, out))
        outs.append(out_file)

    transitive_inputs = [
        kernel_build_infos.ext_module_info.modinst_env.inputs,
    ]

    tools = [
        ctx.executable._check_duplicated_files_in_archives,
        ctx.executable._search_and_cp_output,
    ]
    transitive_tools = [kernel_build_infos.ext_module_info.modinst_env.tools]

    modules_staging_dws = dws.make(ctx, "{}/staging".format(ctx.label.name))

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = kernel_build_infos.ext_module_info.modinst_env,
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )
    command += """
             # create dirs for modules
               mkdir -p {modules_staging_dir}
             # Restore modules_staging_dir from kernel_build
               tar xf {kernel_build_modules_staging_archive} -C {modules_staging_dir}
    """.format(
        modules_staging_dir = modules_staging_dws.directory.path,
        kernel_build_modules_staging_archive =
            kernel_build_infos.ext_module_info.modules_staging_archive.path,
    )
    for input_modules_staging_dws in modules_staging_dws_list:
        # Allow directories to be written because we are merging multiple directories into one.
        # However, don't allow files to be written because we don't expect modules to produce
        # conflicting files. check_duplicated_files_in_archives further enforces this.
        command += dws.restore(
            input_modules_staging_dws,
            dst = modules_staging_dws.directory.path,
            options = "-aL --chmod=D+w",
        )

    # TODO(b/194347374): maybe run depmod.sh with CONFIG_SHELL?
    command += """
             # Check if there are duplicated files in modules_staging_archive of
             # depended kernel_build and kernel_module's
               {check_duplicated_files_in_archives} {modules_staging_archives}
             # Set variables
               if [[ ! -f ${{OUT_DIR}}/include/config/kernel.release ]]; then
                   echo "ERROR: No ${{OUT_DIR}}/include/config/kernel.release" >&2
                   exit 1
               fi
               kernelrelease=$(cat ${{OUT_DIR}}/include/config/kernel.release 2> /dev/null)
               mixed_build_prefix=
               if [[ ${{KBUILD_MIXED_TREE}} ]]; then
                   mixed_build_prefix=${{KBUILD_MIXED_TREE}}/
               fi
               real_modules_staging_dir=$(realpath {modules_staging_dir})
             # Run depmod
               (
                 cd ${{OUT_DIR}} # for System.map when mixed_build_prefix is not set
                 depmod -ae -F "${{mixed_build_prefix}}System.map" -b "${{real_modules_staging_dir}}" ${{kernelrelease}}
               )
             # Remove symlinks that are dead outside of the sandbox
               (
                 find "{modules_staging_dir}/lib/modules/" -maxdepth 2 -mindepth 2 \\( -name source -o -name build \\) -type l -delete > /dev/null
               )
    """.format(
        modules_staging_archives = " ".join(
            [kernel_build_infos.ext_module_info.modules_staging_archive.path] +
            [input_modules_staging_dws.directory.path for input_modules_staging_dws in modules_staging_dws_list],
        ),
        modules_staging_dir = modules_staging_dws.directory.path,
        check_duplicated_files_in_archives = ctx.executable._check_duplicated_files_in_archives.path,
    )

    if external_modules:
        external_module_dir = external_modules[0].dirname
        command += """
                 # Move external modules to declared output location
                   {search_and_cp_output} --srcdir {modules_staging_dir}/lib/modules/*/extra --dstdir {outdir} {filenames}
        """.format(
            modules_staging_dir = modules_staging_dws.directory.path,
            outdir = external_module_dir,
            filenames = " ".join([declared_file.basename for declared_file in external_modules]),
            search_and_cp_output = ctx.executable._search_and_cp_output.path,
        )

    command += """
        # Move additional files to declared output location
        for out in {outs}; do
            cp -pL {modules_staging_dir}/lib/modules/*/${{out}} {outdir}
        done
    """.format(
        modules_staging_dir = modules_staging_dws.directory.path,
        outdir = paths.join(utils.package_bin_dir(ctx), ctx.attr.name),
        outs = " ".join(ctx.attr.outs),
    )

    command += dws.record(modules_staging_dws)

    # --gcov related step
    merge_gcno_step = get_merge_gcno_step(ctx, ctx.attr.kernel_modules)
    inputs += merge_gcno_step.inputs
    outs += merge_gcno_step.outputs
    tools += merge_gcno_step.tools
    command += merge_gcno_step.cmd

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "KernelModulesInstall",
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = depset(tools, transitive = transitive_tools),
        outputs = external_modules + dws.files(modules_staging_dws) + outs,
        command = command,
        progress_message = "Running depmod {}".format(ctx.label),
    )

    # Only analyze headers on external modules.
    # To analyze headers on in-tree modules, just run analyze_inputs on the kernel_build directly.
    cmds_info_targets = ctx.attr.kernel_modules
    cmds_info_srcs = [target[KernelCmdsInfo].srcs for target in cmds_info_targets]
    cmds_info_directories = [target[KernelCmdsInfo].directories for target in cmds_info_targets]
    cmds_info = KernelCmdsInfo(
        srcs = depset(transitive = cmds_info_srcs),
        directories = depset(transitive = cmds_info_directories),
    )

    return [
        DefaultInfo(files = depset(external_modules + outs)),
        KernelModuleInfo(
            kernel_build_infos = kernel_build_infos,
            modules_staging_dws_depset = depset([modules_staging_dws]),
            packages = depset(transitive = [
                target[KernelModuleInfo].packages
                for target in ctx.attr.kernel_modules
            ]),
            label = ctx.label,
            modules_order = depset(transitive = [
                target[KernelModuleInfo].modules_order
                for target in ctx.attr.kernel_modules
            ], order = "postorder"),
        ),
        GcovInfo(
            gcno_mapping = merge_gcno_step.gcno_mapping,
            gcno_dir = merge_gcno_step.gcno_dir,
        ),
        cmds_info,
    ]

kernel_modules_install = rule(
    implementation = _kernel_modules_install_impl,
    doc = """Generates a rule that runs depmod in the module installation directory.

When including this rule to the `data` attribute of a `copy_to_dist_dir` rule,
all external kernel modules specified in `kernel_modules` are included in
distribution.  This excludes `module_outs` in `kernel_build` to avoid conflicts.

Example:
```
kernel_modules_install(
    name = "foo_modules_install",
    kernel_modules = [               # kernel_module rules
        "//path/to/nfc:nfc_module",
    ],
)
kernel_build(
    name = "foo",
    outs = ["vmlinux"],
    module_outs = ["core_module.ko"],
)
copy_to_dist_dir(
    name = "foo_dist",
    data = [
        ":foo",                      # Includes core_module.ko and vmlinux
        ":foo_modules_install",      # Includes nfc_module
    ],
)
```
In `foo_dist`, specifying `foo_modules_install` in `data` won't include
`core_module.ko`, because it is already included in `foo` in `data`.
""",
    attrs = {
        "kernel_modules": attr.label_list(
            providers = [KernelModuleInfo, GcovInfo],
            doc = "A list of labels referring to `kernel_module`s to install.",
        ),
        "kernel_build": attr.label(
            providers = [
                KernelBuildExtModuleInfo,
                # Needed by KernelModuleInfo.kernel_build
                # TODO(b/247622808): Should put the info in KernelModuleInfo directly.
                KernelSerializedEnvInfo,
                KernelBuildInfo,
                KernelImagesInfo,
            ],
            doc = "Label referring to the `kernel_build` module. Otherwise, it" +
                  " is inferred from `kernel_modules`.",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_check_duplicated_files_in_archives": attr.label(
            default = Label("//build/kernel/kleaf:check_duplicated_files_in_archives"),
            doc = "Label referring to the script to process outputs",
            cfg = "exec",
            executable = True,
        ),
        "_search_and_cp_output": attr.label(
            default = Label("//build/kernel/kleaf:search_and_cp_output"),
            cfg = "exec",
            executable = True,
            doc = "Label referring to the script to process outputs",
        ),
        "outs": attr.string_list(
            doc = """ A list of additional outputs from `make modules_install`.

Since external modules are returned by default,
it can be used to obtain modules.* related files (results of depmod).
Only files with allowed names can be added to outs. (`_OUT_ALLOWLIST`)
```
_OUT_ALLOWLIST = {}
```
Example:
```
kernel_modules_install(
    name = "foo_modules_install",
    kernel_modules = [":foo_module_list"],
    outs = [
        "modules.dep",
        "modules.alias",
    ],
)
```
""".format(repr(_OUT_ALLOWLIST)),
        ),
    } | gcov_attrs(),
)
