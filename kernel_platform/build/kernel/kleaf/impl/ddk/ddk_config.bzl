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

"""A target that configures a [`ddk_module`](#ddk_module)."""

load(
    ":common_providers.bzl",
    "KernelBuildExtModuleInfo",
    "KernelSerializedEnvInfo",
)
load(":config_utils.bzl", "config_utils")
load(":ddk/ddk_config_subrule.bzl", "ddk_config_subrule")
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _ddk_config_impl(ctx):
    out_dir = ctx.actions.declare_directory(ctx.attr.name + "/out_dir")
    ddk_config_info = _create_ddk_config_info(ctx)

    _create_main_action(
        ctx = ctx,
        out_dir = out_dir,
        ddk_config_info = ddk_config_info,
    )

    serialized_env_info = _create_serialized_env_info(
        ctx = ctx,
        out_dir = out_dir,
    )

    return [
        DefaultInfo(files = depset([out_dir])),
        serialized_env_info,
        ddk_config_info,
    ]

def _create_merge_dot_config_step(defconfig_depset_written):
    defconfig_depset_file = defconfig_depset_written.depset_file
    cmd = """
        if [[ -s {defconfig_depset_file} ]]; then
            {merge_dot_config_cmd}
        fi
    """.format(
        defconfig_depset_file = defconfig_depset_file.path,
        merge_dot_config_cmd = config_utils.create_merge_dot_config_cmd(
            defconfig_fragments_paths_expr = "$(cat {})".format(defconfig_depset_file.path),
        ),
    )

    return struct(
        inputs = defconfig_depset_written.depset,
        cmd = cmd,
    )

def _create_kconfig_ext_step(ctx, kconfig_depset_written):
    intermediates_dir = utils.intermediates_dir(ctx)
    cmd = """
        mkdir -p {intermediates_dir}

        # Copy all Kconfig files to our new KCONFIG_EXT directory
        if [[ "${{KERNEL_DIR}}/" == "/" ]]; then
            echo "ERROR: FATAL: KERNEL_DIR is not set!" >&2
            exit 1
        fi
        rsync -aL --include="*/" --include="Kconfig*" --exclude="*" ${{KERNEL_DIR}}/${{KCONFIG_EXT_PREFIX}} {intermediates_dir}/

        KCONFIG_EXT_PREFIX=$(realpath {intermediates_dir} --relative-to ${{ROOT_DIR}}/${{KERNEL_DIR}})/

        # Source Kconfig from depending modules
        if [[ -s {kconfig_depset_file} ]]; then
            (
                for kconfig in $(cat {kconfig_depset_file}); do
                    mod_kconfig_rel=$(realpath ${{kconfig}} --relative-to ${{ROOT_DIR}}/${{KERNEL_DIR}})
                    echo 'source "'"${{mod_kconfig_rel}}"'"' >> {intermediates_dir}/Kconfig.ext
                done
            )
        fi
    """.format(
        intermediates_dir = intermediates_dir,
        kconfig_depset_file = kconfig_depset_written.depset_file.path,
    )

    return struct(
        inputs = kconfig_depset_written.depset,
        cmd = cmd,
    )

def _create_oldconfig_step(ctx, defconfig_depset_written, kconfig_depset_written):
    module_label = Label(str(ctx.label).removesuffix("_config"))
    cmd = """
        if [[ -s {defconfig_depset_file} ]] || [[ -s {kconfig_depset_file} ]]; then
            # Regenerate include/.
            # We could also run `make syncconfig` but syncconfig is an implementation detail
            # of Kbuild. Hence, just wipe out include/ to force it to be re-regenerated.
            rm -rf ${{OUT_DIR}}/include

            # Use olddefconfig because we want to use the (new and combined) .config as base, and
            # set unspecified values to their default value.
            make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} \\
                KCONFIG_EXT_PREFIX=${{KCONFIG_EXT_PREFIX}} \\
                olddefconfig
        fi
    """.format(
        defconfig_depset_file = defconfig_depset_written.depset_file.path,
        kconfig_depset_file = kconfig_depset_written.depset_file.path,
    )

    if ctx.file.defconfig:
        cmd += """
            # Check that configs in my defconfig are still there
            # This does not include defconfig from dependencies, because values from
            # dependencies could technically be overridden by this target.
            {check_defconfig_cmd}
        """.format(
            check_defconfig_cmd = config_utils.create_check_defconfig_cmd(module_label, ctx.file.defconfig.path),
        )

    return struct(
        inputs = depset(
            ctx.files.defconfig,
            transitive = [
                defconfig_depset_written.depset,
                kconfig_depset_written.depset,
            ],
        ),
        cmd = cmd,
    )

def _create_main_action(
        ctx,
        out_dir,
        ddk_config_info):
    """Registers the main action that creates the output files."""

    kconfig_depset_written = utils.write_depset(ctx, ddk_config_info.kconfig, "kconfig_depset.txt")
    defconfig_depset_written = utils.write_depset(ctx, ddk_config_info.defconfig, "defconfig_depset.txt")

    ddk_config_env = ctx.attr.kernel_build[KernelBuildExtModuleInfo].ddk_config_env

    transitive_inputs = [
        ddk_config_env.inputs,
    ]

    tools = ddk_config_env.tools

    merge_dot_config_step = _create_merge_dot_config_step(
        defconfig_depset_written = defconfig_depset_written,
    )
    kconfig_ext_step = _create_kconfig_ext_step(
        ctx = ctx,
        kconfig_depset_written = kconfig_depset_written,
    )
    oldconfig_step = _create_oldconfig_step(
        ctx = ctx,
        defconfig_depset_written = defconfig_depset_written,
        kconfig_depset_written = kconfig_depset_written,
    )

    steps = [
        merge_dot_config_step,
        kconfig_ext_step,
        oldconfig_step,
    ]

    for step in steps:
        transitive_inputs.append(step.inputs)

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ddk_config_env,
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )
    command += kernel_utils.set_src_arch_cmd()
    command += """
        {merge_config_cmd}
        {kconfig_ext_cmd}
        {oldconfig_cmd}

        # Copy outputs
        rsync -aL ${{OUT_DIR}}/.config {out_dir}/.config
        rsync -aL ${{OUT_DIR}}/include/ {out_dir}/include/
    """.format(
        merge_config_cmd = merge_dot_config_step.cmd,
        kconfig_ext_cmd = kconfig_ext_step.cmd,
        oldconfig_cmd = oldconfig_step.cmd,
        out_dir = out_dir.path,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = depset(transitive = transitive_inputs),
        tools = tools,
        outputs = [out_dir],
        command = command,
        mnemonic = "DdkConfig",
        progress_message = "Creating DDK module configuration {}".format(ctx.label),
    )

def _create_serialized_env_info(ctx, out_dir):
    """Creates info for module build."""

    # Info from kernel_build
    if ctx.attr.generate_btf:
        # All outputs are required for BTF generation, including vmlinux image
        pre_info = ctx.attr.kernel_build[KernelBuildExtModuleInfo].mod_full_env
    else:
        pre_info = ctx.attr.kernel_build[KernelBuildExtModuleInfo].mod_min_env

    # Overlay module-specific configs
    setup_script_cmd = """
        . {pre_setup_script}
        rsync -aL {out_dir}/.config ${{OUT_DIR}}/.config
        rsync -aL --chmod=D+w {out_dir}/include/ ${{OUT_DIR}}/include/
    """.format(
        pre_setup_script = pre_info.setup_script.path,
        out_dir = out_dir.path,
    )
    setup_script = ctx.actions.declare_file("{name}/{name}_setup.sh".format(name = ctx.attr.name))
    ctx.actions.write(
        output = setup_script,
        content = setup_script_cmd,
    )
    return KernelSerializedEnvInfo(
        setup_script = setup_script,
        inputs = depset([out_dir, setup_script], transitive = [pre_info.inputs]),
        tools = pre_info.tools,
    )

def _create_ddk_config_info(ctx):
    return ddk_config_subrule(
        kconfig_targets = [ctx.attr.kconfig] if ctx.attr.kconfig else [],
        defconfig_targets = [ctx.attr.defconfig] if ctx.attr.defconfig else [],
        deps = ctx.attr.module_deps + ctx.attr.module_hdrs + ctx.attr.module_textual_hdrs,
        extra_defconfigs = ctx.attr.kernel_build[KernelBuildExtModuleInfo].ddk_module_defconfig_fragments,
    )

ddk_config = rule(
    implementation = _ddk_config_impl,
    doc = "A target that configures a [`ddk_module`](#ddk_module).",
    attrs = {
        "kernel_build": attr.label(
            doc = "[`kernel_build`](#kernel_build).",
            providers = [
                KernelBuildExtModuleInfo,
            ],
            mandatory = True,
        ),
        "kconfig": attr.label(
            allow_single_file = True,
            doc = """The `Kconfig` file for this external module.

See
[`Documentation/kbuild/kconfig-language.rst`](https://www.kernel.org/doc/html/latest/kbuild/kconfig.html)
for its format.
""",
        ),
        "defconfig": attr.label(
            allow_single_file = True,
            doc = "The `defconfig` file.",
        ),
        "_write_depset": attr.label(
            default = "//build/kernel/kleaf/impl:write_depset",
            executable = True,
            cfg = "exec",
        ),
        # Needed to compose DdkConfigInfo
        "module_deps": attr.label_list(),
        "module_hdrs": attr.label_list(allow_files = [".h"]),
        "module_textual_hdrs": attr.label_list(allow_files = True),
        "generate_btf": attr.bool(
            default = False,
            doc = "See [kernel_module.generate_btf](#kernel_module-generate_btf)",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
    subrules = [ddk_config_subrule],
)
