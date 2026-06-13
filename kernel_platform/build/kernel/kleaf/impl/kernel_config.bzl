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

"""Creates proper .config and others for kernel_build."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(":abi/trim_nonlisted_kmi_utils.bzl", "trim_nonlisted_kmi_utils")
load(":cache_dir.bzl", "cache_dir")
load(
    ":common_providers.bzl",
    "KernelBuildOriginalEnvInfo",
    "KernelConfigInfo",
    "KernelEnvAttrInfo",
    "KernelEnvInfo",
    "KernelEnvMakeGoalsInfo",
    "KernelSerializedEnvInfo",
    "KernelToolchainInfo",
)
load(":config_utils.bzl", "config_utils")
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":kernel_config_settings.bzl", "kernel_config_settings")
load(":kgdb.bzl", "kgdb")
load(":scripts_config_arg_builder.bzl", _config = "scripts_config_arg_builder")
load(":stamp.bzl", "stamp")
load(":utils.bzl", "kernel_utils")

visibility("//build/kernel/kleaf/...")

# Name of raw symbol list under $OUT_DIR
_RAW_KMI_SYMBOL_LIST_BELOW_OUT_DIR = "abi_symbollist.raw"

def _config_lto(ctx):
    """Return configs for LTO.

    Args:
        ctx: ctx
    Returns:
        a list of arguments to `scripts/config`
    """
    lto_config_flag = ctx.attr.lto

    lto_configs = []

    if lto_config_flag == "fast":
        # buildifier: disable=print
        print("\nWARNING: --lto=fast is deprecated. Falling back to none.")
        lto_config_flag = "none"

    if lto_config_flag == "none":
        lto_configs += [
            _config.disable("LTO_CLANG"),
            _config.enable("LTO_NONE"),
            _config.disable("LTO_CLANG_THIN"),
            _config.disable("LTO_CLANG_FULL"),
            _config.disable("THINLTO"),
            _config.set_val("FRAME_WARN", 0),
        ]
    elif lto_config_flag == "thin":
        lto_configs += [
            _config.enable("LTO_CLANG"),
            _config.disable("LTO_NONE"),
            _config.enable("LTO_CLANG_THIN"),
            _config.disable("LTO_CLANG_FULL"),
            _config.enable("THINLTO"),
        ]
    elif lto_config_flag == "full":
        lto_configs += [
            _config.enable("LTO_CLANG"),
            _config.disable("LTO_NONE"),
            _config.disable("LTO_CLANG_THIN"),
            _config.enable("LTO_CLANG_FULL"),
            _config.disable("THINLTO"),
        ]

    return lto_configs

def _config_trim(ctx):
    """Return configs for trimming.

    Args:
        ctx: ctx
    Returns:
        a list of arguments to `scripts/config`
    """
    if trim_nonlisted_kmi_utils.get_value(ctx) and not ctx.files.raw_kmi_symbol_list:
        fail("{}: trim_nonlisted_kmi is set but raw_kmi_symbol_list is empty.".format(ctx.label))

    if not trim_nonlisted_kmi_utils.get_value(ctx):
        return []

    if ctx.attr._kgdb[BuildSettingInfo].value:
        # buildifier: disable=print
        print("\nWARNING: {this_label}: Symbol trimming \
              IGNORED because --kgdb is set!".format(this_label = ctx.label))
        return []

    if ctx.attr.debug[BuildSettingInfo].value:
        # buildifier: disable=print
        print("\nWARNING: {this_label}: Symbol trimming \
              IGNORED because --debug is set!".format(this_label = ctx.label))
        return []

    return [
        _config.enable("TRIM_UNUSED_KSYMS"),
    ]

def _config_symbol_list(ctx):
    """Return configs for `raw_symbol_list`.

    Args:
        ctx: ctx
    Returns:
        a list of arguments to `scripts/config`
    """
    if not ctx.files.raw_kmi_symbol_list:
        return []

    if len(ctx.files.raw_kmi_symbol_list) > 1:
        fail("{}: raw_kmi_symbol_list must only provide at most one file".format(ctx.label))

    return [
        _config.set_str(
            "UNUSED_KSYMS_WHITELIST",
            _RAW_KMI_SYMBOL_LIST_BELOW_OUT_DIR,
        ),
    ]

def _config_keys(ctx):
    """Return configs for module signing keys and system trusted keys.

    Note: by embedding the system path into the binary, the resulting build
    becomes non-deterministic and the path leaks into the binary. It can be
    discovered with `strings` or even by inspecting the kernel config from the
    binary.

    Args:
        ctx: ctx
    Returns:
        a list of arguments to `scripts/config`
    """
    configs = []
    if ctx.file.module_signing_key:
        configs.append(_config.set_str(
            "MODULE_SIG_KEY",
            ctx.file.module_signing_key.basename,
        ))

    if ctx.file.system_trusted_key:
        configs.append(_config.set_str(
            "SYSTEM_TRUSTED_KEYS",
            ctx.file.system_trusted_key.basename,
        ))

    return configs

def _check_trimming_disabled(ctx):
    """Checks that trimming is disabled if --k*san is set"""
    if not trim_nonlisted_kmi_utils.get_value(ctx):
        return

    for attr_name in (
        "kasan",
        "kasan_sw_tags",
        "kasan_generic",
        "kcsan",
    ):
        if getattr(ctx.attr, attr_name)[BuildSettingInfo].value:
            fail("{}: --{} requires trimming to be disabled".format(ctx.label, attr_name))
    configs = [
        _config.disable("UH"),
        _config.disable("RKP"),
        _config.disable("RKP_TEST"),
    ]

def _reconfig(ctx):
    """Return a command and extra inputs to re-configure `.config` file."""

    _check_trimming_disabled(ctx)

    configs = []
    transitive_deps = []
    apply_defconfig_fragments_cmd = ""
    check_defconfig_fragments_cmd = ""

    for fn in (
        _config_lto,
        _config_trim,
        _config_symbol_list,
        _config_keys,
        kgdb.get_scripts_config_args,
    ):
        configs += fn(ctx)

    if ctx.files.defconfig_fragments:
        transitive_deps += [target.files for target in ctx.attr.defconfig_fragments]
        defconfig_fragments_paths = [f.path for f in ctx.files.defconfig_fragments]

        apply_defconfig_fragments_cmd = config_utils.create_merge_dot_config_cmd(
            " ".join(defconfig_fragments_paths),
        )
        apply_defconfig_fragments_cmd += """
            need_olddefconfig=1
        """

        check_defconfig_fragments_cmd = config_utils.create_check_defconfig_cmd(
            ctx.label,
            " ".join(defconfig_fragments_paths),
        )

    cmd = """
        (
            need_olddefconfig=
            configs_to_apply=$(echo {configs})
            # There could be reconfigurations based on configs which can lead to
            #  an empty `configs_to_apply` even when `configs` is not empty,
            #  for that reason it is better to check it is not empty before using it.
            if [ -n "${{configs_to_apply}}" ]; then
                ${{KERNEL_DIR}}/scripts/config --file ${{OUT_DIR}}/.config ${{configs_to_apply}}
                need_olddefconfig=1
            fi

            {apply_defconfig_fragments_cmd}

            if [[ -n "${{need_olddefconfig}}" ]]; then
                make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} olddefconfig
            fi

            {check_defconfig_fragments_cmd}
        )
    """.format(
        configs = " ".join(configs),
        apply_defconfig_fragments_cmd = apply_defconfig_fragments_cmd,
        check_defconfig_fragments_cmd = check_defconfig_fragments_cmd,
    )

    return struct(
        cmd = cmd,
        deps = depset(transitive = transitive_deps),
    )

def _kernel_config_impl(ctx):
    localversion_file = stamp.write_localversion(ctx)

    inputs = [
        s
        for s in ctx.files.srcs
        if any([token in s.path for token in [
            "Kbuild",
            "Kconfig",
            "Makefile",
            "configs/",
            "scripts/",
            ".fragment",
        ]])
    ]
    transitive_inputs = []

    out_dir = ctx.actions.declare_directory(ctx.attr.name + "/out_dir")
    outputs = [out_dir]

    reconfig = _reconfig(ctx)
    transitive_inputs.append(reconfig.deps)

    tools = []

    transitive_inputs.append(ctx.attr.env[KernelEnvInfo].inputs)
    transitive_tools = [ctx.attr.env[KernelEnvInfo].tools]

    cache_dir_step = cache_dir.get_step(
        ctx = ctx,
        common_config_tags = ctx.attr.env[KernelEnvAttrInfo].common_config_tags,
        symlink_name = "config",
    )
    inputs += cache_dir_step.inputs
    outputs += cache_dir_step.outputs
    tools += cache_dir_step.tools

    inputs.append(localversion_file)

    sync_raw_kmi_symbol_list_cmd = ""
    if ctx.files.raw_kmi_symbol_list:
        sync_raw_kmi_symbol_list_cmd = """
            rsync -aL {raw_kmi_symbol_list} {out_dir}/{raw_kmi_symbol_list_below_out_dir}
        """.format(
            out_dir = out_dir.path,
            raw_kmi_symbol_list = ctx.files.raw_kmi_symbol_list[0].path,
            raw_kmi_symbol_list_below_out_dir = _RAW_KMI_SYMBOL_LIST_BELOW_OUT_DIR,
        )
        inputs += ctx.files.raw_kmi_symbol_list

    # exclude keys in out_dir to avoid accidentally including them
    # in the distribution.

    command = ctx.attr.env[KernelEnvInfo].setup + """
          {cache_dir_cmd}
        # Pre-defconfig commands
          eval ${{PRE_DEFCONFIG_CMDS}}
        # Actual defconfig
          make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} ${{DEFCONFIG}}
        # Post-defconfig commands
          eval ${{POST_DEFCONFIG_CMDS}}
        # Re-config
          {reconfig_cmd}
        # HACK: run syncconfig to avoid re-triggerring kernel_build
          make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} syncconfig
        # Grab outputs
          rsync -aL ${{OUT_DIR}}/.config {out_dir}/.config
          rsync -aL ${{OUT_DIR}}/include/ {out_dir}/include/
          rsync -aL {localversion_file} {out_dir}/localversion
          {sync_raw_kmi_symbol_list_cmd}

        # Ensure reproducibility. The value of the real $ROOT_DIR is replaced in the setup script.
          sed -i'' -e 's:'"${{ROOT_DIR}}"':${{ROOT_DIR}}:g' {out_dir}/include/config/auto.conf.cmd

        # HACK: Ensure we always SYNC auto.conf. This ensures binaries like fixdep are always
        # re-built. See b/263415662
          echo "include/config/auto.conf: FORCE" >> {out_dir}/include/config/auto.conf.cmd

          {cache_dir_post_cmd}
        """.format(
        out_dir = out_dir.path,
        cache_dir_cmd = cache_dir_step.cmd,
        cache_dir_post_cmd = cache_dir_step.post_cmd,
        reconfig_cmd = reconfig.cmd,
        localversion_file = localversion_file.path,
        sync_raw_kmi_symbol_list_cmd = sync_raw_kmi_symbol_list_cmd,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "KernelConfig",
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = outputs,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Creating kernel config {}{}".format(
            ctx.attr.env[KernelEnvAttrInfo].progress_message_note,
            ctx.label,
        ),
        command = command,
        execution_requirements = kernel_utils.local_exec_requirements(ctx),
    )

    post_setup_deps = [out_dir]

    extra_restore_outputs_cmd = ""
    for file in (ctx.file.module_signing_key, ctx.file.system_trusted_key):
        if not file:
            continue
        extra_restore_outputs_cmd += """
            rsync -aL {file} ${{OUT_DIR}}/{basename}
        """.format(
            file = file.path,
            basename = file.basename,
        )
        post_setup_deps.append(file)

    # <kernel_build>_config_setup.sh
    serialized_env_info_setup_script = ctx.actions.declare_file("{name}/{name}_setup.sh".format(name = ctx.attr.name))
    ctx.actions.write(
        output = serialized_env_info_setup_script,
        content = get_config_setup_command(
            env_setup_command = ctx.attr.env[KernelEnvInfo].setup,
            out_dir = out_dir,
            extra_restore_outputs_cmd = extra_restore_outputs_cmd,
        ),
    )

    serialized_env_info = KernelSerializedEnvInfo(
        setup_script = serialized_env_info_setup_script,
        tools = ctx.attr.env[KernelEnvInfo].tools,
        inputs = depset(post_setup_deps + [
            serialized_env_info_setup_script,
        ], transitive = transitive_inputs),
    )

    config_script_ret = _get_config_script(ctx, inputs)

    return [
        serialized_env_info,
        ctx.attr.env[KernelEnvAttrInfo],
        ctx.attr.env[KernelEnvMakeGoalsInfo],
        ctx.attr.env[KernelToolchainInfo],
        KernelBuildOriginalEnvInfo(
            env_info = ctx.attr.env[KernelEnvInfo],
        ),
        DefaultInfo(
            files = depset([out_dir]),
            executable = config_script_ret.executable,
            runfiles = config_script_ret.runfiles,
        ),
        KernelConfigInfo(
            env_setup_script = ctx.file.env,
        ),
    ]

def _get_config_script(ctx, inputs):
    """Handles config.sh."""
    executable = ctx.actions.declare_file("{}/config.sh".format(ctx.attr.name))

    script = ctx.attr.env[KernelEnvInfo].run_env.setup

    # TODO(b/254348147): Support ncurses for hermetic tools
    script += """
          export HOSTCFLAGS="${HOSTCFLAGS} --sysroot="
          export HOSTLDFLAGS="${HOSTLDFLAGS} --sysroot="
    """

    script += kernel_utils.set_src_arch_cmd()

    script += """
            menucommand="${1:-savedefconfig}"
            if ! [[ "${menucommand}" =~ .*config ]]; then
                echo "Invalid command $menucommand. Must be *config." >&2
                exit 1
            fi

            # Pre-defconfig commands
            set -x
            eval ${PRE_DEFCONFIG_CMDS}
            set +x
            # Actual defconfig
            make -C ${KERNEL_DIR} ${TOOL_ARGS} O=${OUT_DIR} ${DEFCONFIG}

            # Show UI
            menuconfig ${menucommand}

            # Post-defconfig commands
            set -x
            eval ${POST_DEFCONFIG_CMDS}
            set +x
    """

    ctx.actions.write(
        output = executable,
        content = script,
        is_executable = True,
    )

    runfiles = ctx.runfiles(
        files = inputs,
        transitive_files = depset(transitive = [
            ctx.attr.env[KernelEnvInfo].run_env.inputs,
            ctx.attr.env[KernelEnvInfo].run_env.tools,
        ]),
    )

    return struct(
        executable = executable,
        runfiles = runfiles,
    )

def get_config_setup_command(
        env_setup_command,
        out_dir,
        extra_restore_outputs_cmd):
    """Returns the content of `<kernel_build>_config_setup.sh`, given the parameters.

    Args:
        env_setup_command: command to set up environment from `kernel_env`
        out_dir: output directory from `kernel_config`
        extra_restore_outputs_cmd: Extra CMD to restore outputs
    Returns:
        the command to setup the environment like after `make defconfig`.
    """

    cmd = """
        {env_setup_command}
        {eval_restore_out_dir_cmd}

        [ -z ${{OUT_DIR}} ] && echo "FATAL: configs post_env_info setup run without OUT_DIR set!" >&2 && exit 1
        # Restore kernel config inputs
        mkdir -p ${{OUT_DIR}}/include/
        rsync -aL {out_dir}/.config ${{OUT_DIR}}/.config
        rsync -aL --chmod=D+w {out_dir}/include/ ${{OUT_DIR}}/include/
        rsync -aL --chmod=F+w {out_dir}/localversion ${{OUT_DIR}}/localversion
        if [[ -f {out_dir}/{raw_kmi_symbol_list_below_out_dir} ]]; then
            rsync -aL --chmod=F+w \\
                {out_dir}/{raw_kmi_symbol_list_below_out_dir} ${{OUT_DIR}}/
        fi

        # Restore real value of $ROOT_DIR in auto.conf.cmd
        sed -i'' -e 's:${{ROOT_DIR}}:'"${{ROOT_DIR}}"':g' ${{OUT_DIR}}/include/config/auto.conf.cmd
    """.format(
        env_setup_command = env_setup_command,
        eval_restore_out_dir_cmd = kernel_utils.eval_restore_out_dir_cmd(),
        out_dir = out_dir.path,
        raw_kmi_symbol_list_below_out_dir = _RAW_KMI_SYMBOL_LIST_BELOW_OUT_DIR,
    )
    cmd += extra_restore_outputs_cmd
    return cmd

def _kernel_config_additional_attrs():
    return dicts.add(
        kernel_config_settings.of_kernel_config(),
        cache_dir.attrs(),
    )

kernel_config = rule(
    implementation = _kernel_config_impl,
    doc = """Defines a kernel config target.

- When `bazel build <target>`, this target runs `make defconfig` etc. during the build.
- When `bazel run <target> -- Xconfig`, this target runs `make Xconfig`.
""",
    attrs = {
        "env": attr.label(
            mandatory = True,
            providers = [
                KernelEnvInfo,
                KernelEnvAttrInfo,
                KernelEnvMakeGoalsInfo,
                KernelToolchainInfo,
            ],
            doc = "environment target that defines the kernel build environment",
            allow_single_file = True,
        ),
        "srcs": attr.label_list(mandatory = True, doc = "kernel sources", allow_files = True),
        "raw_kmi_symbol_list": attr.label(
            doc = "Label to abi_symbollist.raw. Must be 0 or 1 file.",
            allow_files = True,
        ),
        "module_signing_key": attr.label(
            doc = "Label to module signing key.",
            allow_single_file = True,
        ),
        "system_trusted_key": attr.label(
            doc = "Label to trusted system key.",
            allow_single_file = True,
        ),
        "defconfig_fragments": attr.label_list(
            doc = "defconfig fragments",
            allow_files = True,
        ),
        "_write_depset": attr.label(
            default = "//build/kernel/kleaf/impl:write_depset",
            executable = True,
            cfg = "exec",
        ),
        "_config_is_stamp": attr.label(default = "//build/kernel/kleaf:config_stamp"),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    } | _kernel_config_additional_attrs(),
    executable = True,
    toolchains = [hermetic_toolchain.type],
)
