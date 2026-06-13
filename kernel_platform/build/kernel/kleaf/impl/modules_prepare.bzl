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

"""Runs `make modules_prepare` to prepare `$OUT_DIR` for modules."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load(":cache_dir.bzl", "cache_dir")
load(
    ":common_providers.bzl",
    "KernelEnvAttrInfo",
    "KernelSerializedEnvInfo",
)
load(":debug.bzl", "debug")
load(":kernel_config_settings.bzl", "kernel_config_settings")
load(":utils.bzl", "kernel_utils")

visibility("//build/kernel/kleaf/...")

def _modules_prepare_impl(ctx):
    inputs = []
    tools = []
    transitive_tools = []
    transitive_inputs = []

    transitive_inputs += [target.files for target in ctx.attr.srcs]

    outputs = [ctx.outputs.outdir_tar_gz]

    transitive_tools.append(ctx.attr.config[KernelSerializedEnvInfo].tools)
    transitive_inputs.append(ctx.attr.config[KernelSerializedEnvInfo].inputs)

    cache_dir_step = cache_dir.get_step(
        ctx = ctx,
        common_config_tags = ctx.attr.config[KernelEnvAttrInfo].common_config_tags,
        symlink_name = "modules_prepare",
    )
    inputs += cache_dir_step.inputs
    outputs += cache_dir_step.outputs
    tools += cache_dir_step.tools

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.config[KernelSerializedEnvInfo],
        restore_out_dir_cmd = cache_dir_step.cmd,
    )

    force_gen_headers_cmd = ""
    if ctx.attr.force_generate_headers:
        force_gen_headers_cmd += """
        # Workaround to force the creation of these missing files.
           mkdir -p ${OUT_DIR}/security/selinux/
           ${OUT_DIR}/scripts/selinux/genheaders/genheaders ${OUT_DIR}/security/selinux/flask.h ${OUT_DIR}/security/selinux/av_permissions.h
        """

    command += """
         # Prepare for the module build
           make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} KERNEL_SRC=${{ROOT_DIR}}/${{KERNEL_DIR}} modules_prepare
         # Additional steps
           {force_gen_headers_cmd}
         # b/279211056: Exclude the top-level source symlink. It is not useful and points outside
         # of the directory, making tar unhappy.
           rm -f ${{OUT_DIR}}/source
         # Package files
           tar czf {outdir_tar_gz} -C ${{OUT_DIR}} .
           {cache_dir_post_cmd}
    """.format(
        force_gen_headers_cmd = force_gen_headers_cmd,
        outdir_tar_gz = ctx.outputs.outdir_tar_gz.path,
        cache_dir_post_cmd = cache_dir_step.post_cmd,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "ModulesPrepare",
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = outputs,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Preparing for module build {}{}".format(
            ctx.attr.config[KernelEnvAttrInfo].progress_message_note,
            ctx.label,
        ),
        command = command,
        execution_requirements = kernel_utils.local_exec_requirements(ctx),
    )

    setup_script_cmd = modules_prepare_setup_command(
        config_setup_script = ctx.attr.config[KernelSerializedEnvInfo].setup_script,
        modules_prepare_outdir_tar_gz = ctx.outputs.outdir_tar_gz,
    )

    # <kernel_build>_modules_prepare_setup.sh
    setup_script = ctx.actions.declare_file("{name}/{name}_setup.sh".format(name = ctx.attr.name))
    ctx.actions.write(
        output = setup_script,
        content = setup_script_cmd,
    )

    return [
        KernelSerializedEnvInfo(
            setup_script = setup_script,
            inputs = depset(
                [ctx.outputs.outdir_tar_gz, setup_script],
                transitive = [ctx.attr.config[KernelSerializedEnvInfo].inputs],
            ),
            tools = ctx.attr.config[KernelSerializedEnvInfo].tools,
        ),
        DefaultInfo(files = depset([ctx.outputs.outdir_tar_gz, setup_script])),
    ]

def modules_prepare_setup_command(
        config_setup_script,
        modules_prepare_outdir_tar_gz):
    return """
        source {config_setup_script}
        # Restore modules_prepare outputs. Assumes env setup.
        [ -z ${{OUT_DIR}} ] && echo "ERROR: modules_prepare setup run without OUT_DIR set!" >&2 && exit 1
        mkdir -p ${{OUT_DIR}}
        tar xf {modules_prepare_outdir_tar_gz} -C ${{OUT_DIR}}
    """.format(
        config_setup_script = config_setup_script.path,
        modules_prepare_outdir_tar_gz = modules_prepare_outdir_tar_gz.path,
    )

def _modules_prepare_additional_attrs():
    return dicts.add(
        kernel_config_settings.of_modules_prepare(),
        cache_dir.attrs(),
    )

modules_prepare = rule(
    doc = "Rule that runs `make modules_prepare` to prepare `$OUT_DIR` for modules.",
    implementation = _modules_prepare_impl,
    attrs = {
        "config": attr.label(
            mandatory = True,
            providers = [KernelEnvAttrInfo, KernelSerializedEnvInfo],
            doc = "the kernel_config target",
        ),
        "srcs": attr.label_list(mandatory = True, doc = "kernel sources", allow_files = True),
        "outdir_tar_gz": attr.output(
            mandatory = True,
            doc = "the packaged ${OUT_DIR} files",
        ),
        "force_generate_headers": attr.bool(
            doc = "If True it forces generation of additional headers after make modules_prepare",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    } | _modules_prepare_additional_attrs(),
)
