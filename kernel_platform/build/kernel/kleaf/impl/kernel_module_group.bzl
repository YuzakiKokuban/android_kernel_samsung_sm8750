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

"""A group of external kernel modules."""

load(
    ":common_providers.bzl",
    "GcovInfo",
    "KernelCmdsInfo",
    "KernelModuleInfo",
    "KernelModuleSetupInfo",
    "KernelUnstrippedModulesInfo",
    "ModuleSymversInfo",
)
load(":ddk/ddk_headers.bzl", "DdkHeadersInfo", "ddk_headers_common_impl")
load(":gcov_utils.bzl", "gcov_attrs", "get_merge_gcno_step")
load(":utils.bzl", "kernel_utils")

visibility("//build/kernel/kleaf/...")

def _kernel_module_group_impl(ctx):
    targets = ctx.attr.srcs
    merge_gcno_step = get_merge_gcno_step(ctx, targets)

    # As of Aug 2024, this is only needed to merge --gcov outputs.
    # It can be generalized if needed later.
    if merge_gcno_step.outputs:
        ctx.actions.run_shell(
            mnemonic = "KernelModuleGroupOutputs",
            inputs = merge_gcno_step.inputs,
            tools = merge_gcno_step.tools,
            outputs = merge_gcno_step.outputs,
            command = merge_gcno_step.cmd,
            progress_message = "Merging gcno directories %{label}",
        )
    gcov_info = GcovInfo(
        gcno_mapping = merge_gcno_step.gcno_mapping,
        gcno_dir = merge_gcno_step.gcno_dir,
    )
    default_info = DefaultInfo(
        files = depset(merge_gcno_step.outputs, transitive = [target.files for target in targets]),
        runfiles = ctx.runfiles().merge_all([target[DefaultInfo].default_runfiles for target in targets]),
    )

    setup_transitive_inputs = []
    setup_cmds = []
    for target in targets:
        setup_transitive_inputs.append(target[KernelModuleSetupInfo].inputs)
        setup_cmds.append(target[KernelModuleSetupInfo].setup)
    setup_info = KernelModuleSetupInfo(
        inputs = depset(transitive = setup_transitive_inputs),
        setup = "\n".join(setup_cmds),
    )

    kernel_utils.check_kernel_build(
        [target[KernelModuleInfo] for target in targets],
        None,
        ctx.label,
    )
    kernel_module_info = KernelModuleInfo(
        kernel_build_infos = targets[0][KernelModuleInfo].kernel_build_infos,
        modules_staging_dws_depset = depset(transitive = [
            target[KernelModuleInfo].modules_staging_dws_depset
            for target in targets
        ]),
        kernel_uapi_headers_dws_depset = depset(transitive = [
            target[KernelModuleInfo].kernel_uapi_headers_dws_depset
            for target in targets
        ]),
        files = depset(transitive = [
            target[KernelModuleInfo].files
            for target in targets
        ]),
        packages = depset(transitive = [target[KernelModuleInfo].packages for target in targets]),
        label = ctx.label,
        modules_order = depset(
            transitive = [target[KernelModuleInfo].modules_order for target in targets],
            order = "postorder",
        ),
    )

    unstripped_modules_info = KernelUnstrippedModulesInfo(
        directories = depset(transitive = [
            target[KernelUnstrippedModulesInfo].directories
            for target in targets
        ], order = "postorder"),
    )

    module_symvers_info = ModuleSymversInfo(
        restore_paths = depset(transitive = [
            target[ModuleSymversInfo].restore_paths
            for target in targets
        ]),
    )

    ddk_headers_info = ddk_headers_common_impl(ctx.label, targets, [], [])

    cmds_info_srcs = [target[KernelCmdsInfo].srcs for target in targets]
    cmds_info_directories = [target[KernelCmdsInfo].directories for target in targets]
    cmds_info = KernelCmdsInfo(
        srcs = depset(transitive = cmds_info_srcs),
        directories = depset(transitive = cmds_info_directories),
    )

    # Sync list of infos with kernel_module / ddk_module.
    return [
        default_info,
        setup_info,
        kernel_module_info,
        unstripped_modules_info,
        module_symvers_info,
        ddk_headers_info,
        cmds_info,
        gcov_info,
    ]

kernel_module_group = rule(
    implementation = _kernel_module_group_impl,
    doc = """Like filegroup but for [`kernel_module`](#kernel_module)s or [`ddk_module`](#ddk_module)s.

Unlike filegroup, `srcs` must not be empty.

Example:

```
# //package/my_subsystem

# Hide a.ko and b.ko because they are implementation details of my_subsystem
ddk_module(
    name = "a",
    visibility = ["//visibility:private"],
    ...
)

ddk_module(
    name = "b",
    visibility = ["//visibility:private"],
    ...
)

# my_subsystem is the public target that the device should depend on.
kernel_module_group(
    name = "my_subsystem",
    srcs = [":a", ":b"],
    visibility = ["//package/my_device:__subpackages__"],
)

# //package/my_device
kernel_modules_install(
    name = "my_device_modules_install",
    kernel_modules = [
        "//package/my_subsystem:my_subsystem", # This is equivalent to specifying a and b.
    ],
)
```
    """,
    attrs = {
        "srcs": attr.label_list(
            doc = "List of [`kernel_module`](#kernel_module)s or [`ddk_module`](#ddk_module)s.",
            mandatory = True,
            providers = [
                DdkHeadersInfo,
                DefaultInfo,
                GcovInfo,
                KernelModuleSetupInfo,
                KernelModuleInfo,
                KernelUnstrippedModulesInfo,
                ModuleSymversInfo,
                KernelCmdsInfo,
            ],
        ),
    } | gcov_attrs(),
)
