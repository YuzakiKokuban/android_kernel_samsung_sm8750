# Copyright (C) 2024 The Android Open Source Project
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

"""Helper functions to handle GCOV files."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(":abi/base_kernel_utils.bzl", "base_kernel_utils")
load(":common_providers.bzl", "GcovInfo")

visibility("//build/kernel/kleaf/...")

def _gcno_common_impl(ctx, file_mappings_args, rsync_cmd, mappings_args, extra_inputs, gcno_dir):
    """Returns a step for handling `*.gcno`files and mappings.

    Args:
        ctx: Context from the rule.
        file_mappings_args: Additional file mappings, separated by spaces.
        rsync_cmd: All directories rsync commands.
        mappings_args: SRC:DEST mappings, separated by spaces.
        extra_inputs: inputs.
        gcno_dir: Target destination.

    Returns:
      A struct with fields (inputs, tools, outputs, cmd, gcno_mapping, gcno_dir)
    """
    grab_gcno_cmd = ""
    inputs = extra_inputs
    outputs = []
    tools = []
    gcno_mapping = None

    if ctx.attr._gcov[BuildSettingInfo].value:
        gcno_mapping = ctx.actions.declare_file("{name}/gcno_mapping.{name}.json".format(name = ctx.label.name))
        gcno_archive = ctx.actions.declare_file(
            "{name}/{name}.gcno.tar.gz".format(name = ctx.label.name),
        )
        outputs += [gcno_dir, gcno_mapping, gcno_archive]
        tools.append(ctx.executable._print_gcno_mapping)

        grab_gcno_cmd = """
            # Sync directories.
            {rsync_cmd}
            # Merge mappings.
            {print_gcno_mapping} {file_mappings_args} {mappings_args} > {gcno_mapping}
            # Archive gcno_dir + gcno_mapping + base_kernel_gcno_dir
            cp {gcno_mapping} {gcno_dir}
            tar czf {gcno_archive} -C {gcno_dir} .
        """.format(
            rsync_cmd = rsync_cmd,
            gcno_dir = gcno_dir.path,
            gcno_mapping = gcno_mapping.path,
            print_gcno_mapping = ctx.executable._print_gcno_mapping.path,
            file_mappings_args = file_mappings_args,
            mappings_args = mappings_args,
            gcno_archive = gcno_archive.path,
        )
    return struct(
        inputs = inputs,
        tools = tools,
        cmd = grab_gcno_cmd,
        outputs = outputs,
        gcno_mapping = gcno_mapping,
        gcno_dir = gcno_dir,
    )

def get_grab_gcno_step(ctx, src_dir, is_kernel_build):
    """Returns a step for grabbing the `*.gcno`files from `src_dir`.

    Args:
        ctx: Context from the rule.
        src_dir: Source directory.
        is_kernel_build: The flag to indicate whether the rule is `kernel_build`.

    Returns:
      A struct with fields (inputs, tools, outputs, cmd, gcno_mapping, gcno_dir)
    """

    if not ctx.attr._gcov[BuildSettingInfo].value:
        return _gcno_common_impl(ctx, "", "", "", [], None)

    file_mappings_args = ""
    inputs = []
    mappings_args = ""
    rsync_cmd = ""

    gcno_dir = ctx.actions.declare_directory("{name}/{name}_gcno".format(name = ctx.label.name))
    base_kernel = ""
    if is_kernel_build == True:
        base_kernel = base_kernel_utils.get_base_kernel(ctx)
    if base_kernel and base_kernel[GcovInfo].gcno_mapping:
        file_mappings_args = "--file_mappings {}".format(base_kernel[GcovInfo].gcno_mapping.path)
        inputs.append(base_kernel[GcovInfo].gcno_mapping)
        if base_kernel[GcovInfo].gcno_dir:
            inputs.append(base_kernel[GcovInfo].gcno_dir)
            rsync_cmd += """
                # Copy all *.gcno files and its subdirectories recursively.
                rsync -a -L --prune-empty-dirs --include '*/' --include '*.gcno' --exclude '*' {base_gcno_dir}/ {gcno_dir}/
            """.format(
                base_gcno_dir = base_kernel[GcovInfo].gcno_dir.path,
                gcno_dir = gcno_dir.path,
            )

    # Note: Emitting `src_dir` is one source of ir-reproducible output for sandbox actions.
    # However, note that these ir-reproducibility are tied to vmlinux, because these paths are already
    # embedded in vmlinux. This file just makes such ir-reproducibility more explicit.
    rsync_cmd += """
        rsync -a -L --prune-empty-dirs --include '*/' --include '*.gcno' --exclude '*' {src_dir}/ {gcno_dir}/
    """.format(
        src_dir = src_dir,
        gcno_dir = gcno_dir.path,
    )
    mappings_args = "--mappings {src_dir}:{gcno_dir}".format(src_dir = src_dir, gcno_dir = gcno_dir.path)

    return _gcno_common_impl(ctx, file_mappings_args, rsync_cmd, mappings_args, inputs, gcno_dir)

def get_merge_gcno_step(ctx, targets):
    """Returns a step for merging `*.gcno`directories and their mappings.

    Args:
        ctx: Context from the rule.
        targets: The input labels from where to get the directories and mappings.

    Returns:
      A struct with fields (inputs, tools, outputs, cmd, gcno_mapping, gcno_dir)
    """

    if not ctx.attr._gcov[BuildSettingInfo].value:
        return _gcno_common_impl(ctx, "", "", "", [], None)

    file_mappings_cmd = ""
    inputs = []
    mappings_cmd = ""
    rsync_cmd = ""
    gcno_dir = ctx.actions.declare_directory("{name}/{name}_gcno".format(name = ctx.label.name))
    for target in targets:
        if GcovInfo not in target:
            continue
        if target[GcovInfo].gcno_mapping:
            inputs.append(target[GcovInfo].gcno_mapping)
            if not file_mappings_cmd:
                file_mappings_cmd += "--file_mappings"
            file_mappings_cmd += " {}".format(target[GcovInfo].gcno_mapping.path)
        if target[GcovInfo].gcno_dir:
            inputs.append(target[GcovInfo].gcno_dir)
            rsync_cmd += """
                # Copy all *.gcno files and its subdirectories recursively.
                rsync -a -L --prune-empty-dirs --include '*/' --include '*.gcno' --exclude '*' {target_gcno_dir}/ {gcno_dir}/
            """.format(
                target_gcno_dir = target[GcovInfo].gcno_dir.path,
                gcno_dir = gcno_dir.path,
            )

    return _gcno_common_impl(ctx, file_mappings_cmd, rsync_cmd, mappings_cmd, inputs, gcno_dir)

def gcov_attrs():
    return {
        "_print_gcno_mapping": attr.label(
            default = Label("//build/kernel/kleaf/impl:print_gcno_mapping"),
            cfg = "exec",
            executable = True,
        ),
        "_gcov": attr.label(default = "//build/kernel/kleaf:gcov"),
    }
