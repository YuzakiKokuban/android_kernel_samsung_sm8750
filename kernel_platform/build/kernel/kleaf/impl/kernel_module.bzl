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

"""An external kernel module.

Makefile and Kbuild files are required.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//build/kernel/kleaf:directory_with_structure.bzl", dws = "directory_with_structure")
load(
    "//build/kernel/kleaf/artifact_tests:kernel_test.bzl",
    "kernel_module_test",
)
load(":cache_dir.bzl", "cache_dir")
load(
    ":common_providers.bzl",
    "DdkConfigInfo",
    "DdkSubmoduleInfo",
    "GcovInfo",
    "KernelBuildExtModuleInfo",
    "KernelCmdsInfo",
    "KernelEnvAttrInfo",
    "KernelModuleInfo",
    "KernelModuleSetupInfo",
    "KernelSerializedEnvInfo",
    "KernelUnstrippedModulesInfo",
    "ModuleSymversInfo",
)
load(":ddk/ddk_headers.bzl", "DdkHeadersInfo")
load(":debug.bzl", "debug")
load(":gcov_utils.bzl", "gcov_attrs", "get_grab_gcno_step")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":kernel_build.bzl", "get_grab_cmd_step")
load(":stamp.bzl", "stamp")
load(":utils.bzl", "kernel_utils")

visibility("//build/kernel/kleaf/...")

def kernel_module(
        name,
        kernel_build,
        outs = None,
        srcs = None,
        deps = None,
        makefile = None,
        generate_btf = None,
        **kwargs):
    """Generates a rule that builds an external kernel module.

    Example:
    ```
    kernel_module(
        name = "nfc",
        srcs = glob([
            "**/*.c",
            "**/*.h",

            # If there are Kbuild files, add them
            "**/Kbuild",
            # If there are additional makefiles in subdirectories, add them
            "**/Makefile",
        ]),
        outs = ["nfc.ko"],
        kernel_build = "//common:kernel_aarch64",
    )
    ```

    Args:
        name: Name of this kernel module.
        srcs: Source files to build this kernel module. If unspecified or value
          is `None`, it is by default the list in the above example:
          ```
          glob([
            "**/*.c",
            "**/*.h",
            "**/Kbuild",
            "**/Makefile",
          ])
          ```
        kernel_build: Label referring to the kernel_build module.
        deps: A list of other `kernel_module` or `ddk_module` dependencies.

          Before building this target, `Modules.symvers` from the targets in
          `deps` are restored, so this target can be built against
          them.

          It is an undefined behavior to put targets of other types to this list
          (e.g. `ddk_headers`).
        outs: The expected output files. If unspecified or value is `None`, it
          is `["{name}.ko"]` by default.

          For each token `out`, the build rule automatically finds a
          file named `out` in the legacy kernel modules staging
          directory. The file is copied to the output directory of
          this package, with the label `name/out`.

          - If `out` doesn't contain a slash, subdirectories are searched.

            Example:
            ```
            kernel_module(name = "nfc", outs = ["nfc.ko"])
            ```

            The build system copies
            ```
            <legacy modules staging dir>/lib/modules/*/extra/<some subdir>/nfc.ko
            ```
            to
            ```
            <package output dir>/nfc.ko
            ```

            `nfc/nfc.ko` is the label to the file.

          - If `out` contains slashes, its value is used. The file is
            also copied to the top of package output directory.

            For example:
            ```
            kernel_module(name = "nfc", outs = ["foo/nfc.ko"])
            ```

            The build system copies
            ```
            <legacy modules staging dir>/lib/modules/*/extra/foo/nfc.ko
            ```
            to
            ```
            foo/nfc.ko
            ```

            `nfc/foo/nfc.ko` is the label to the file.

            The file is also copied to `<package output dir>/nfc.ko`.

            `nfc/nfc.ko` is the label to the file.

            See `search_and_cp_output.py` for details.
        makefile: `Makefile` for the module. By default, this is `Makefile` in the current package.

            This file determines where `make modules` is executed.

            This is useful when the Makefile is located in a different package or in a subdirectory.
        generate_btf: Allows generation of BTF type information for the module.
          If enabled, passes `vmlinux` image to module build, which is required
          by BTF generator makefile scripts.

          Disabled by default.

          Requires `CONFIG_DEBUG_INFO_BTF` enabled in base kernel.

          Requires rebuild of module if `vmlinux` changed, which may lead to an
          increase of incremental build time.

          BTF type information increases size of module binary.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """

    if kwargs.get("kernel_module_deps"):
        fail("{}: kernel_module_deps is deprecated. Use deps instead.".format(
            native.package_relative_label(name),
        ))

    kwargs.update(
        # This should be the exact list of arguments of kernel_module.
        # Default arguments of _kernel_module go into _kernel_module_set_defaults.
        name = name,
        srcs = srcs,
        kernel_build = kernel_build,
        deps = deps,
        outs = outs,
        makefile = makefile,
        generate_btf = generate_btf,
    )
    kwargs = _kernel_module_set_defaults(kwargs)

    main_kwargs = dict(kwargs)
    main_kwargs["name"] = name
    main_kwargs["outs"] = ["{name}/{out}".format(name = name, out = out) for out in main_kwargs["outs"]]
    _kernel_module(**main_kwargs)

    kernel_module_test(
        name = name + "_test",
        modules = [name],
        tags = kwargs.get("tags"),
    )

def _check_module_symvers_restore_path(kernel_modules, this_label):
    all_restore_paths = dict()
    for kernel_module in kernel_modules:
        for restore_path in kernel_module.module_symvers_info.restore_paths.to_list():
            if restore_path not in all_restore_paths:
                all_restore_paths[restore_path] = []
            all_restore_paths[restore_path].append(str(kernel_module.label))

    dups = dict()
    for key, values in all_restore_paths.items():
        if len(values) > 1:
            dups[key] = values

    if dups:
        fail("""{this_label}: Conflicting dependencies. Dependencies from a package must either be a list of `ddk_module`s only, or a single `kernel_module`.
{conflicts}
        """.format(
            this_label = this_label,
            conflicts = json.encode_indent(list(dups.values()), indent = "  "),
        ))

def _get_implicit_outs(ctx):
    """Gets the list of implicit output files from makefile targets."""
    implicit_outs = ctx.attr.internal_ddk_makefiles_dir[DdkSubmoduleInfo].outs.to_list()

    implicit_outs_to_srcs = {}
    for implicit_out in implicit_outs:
        if implicit_out.out not in implicit_outs_to_srcs:
            implicit_outs_to_srcs[implicit_out.out] = []
        implicit_outs_to_srcs[implicit_out.out].append(implicit_out.src)

    duplicated_implicit_outs = {}
    for out, srcs in implicit_outs_to_srcs.items():
        if len(srcs) > 1:
            duplicated_implicit_outs[out] = srcs

    if duplicated_implicit_outs:
        fail("{}: Multiple submodules define the same output file: {}".format(
            ctx.label,
            json.encode_indent(duplicated_implicit_outs, indent = "  "),
        ))

    return list(implicit_outs_to_srcs.keys())

def _kernel_module_impl(ctx):
    split_deps = kernel_utils.split_kernel_module_deps(ctx.attr.deps, ctx.label)
    kernel_module_deps = split_deps.kernel_modules
    kernel_module_deps = [kernel_utils.create_kernel_module_dep_info(target) for target in kernel_module_deps]
    if ctx.attr.internal_ddk_makefiles_dir:
        kernel_module_deps += ctx.attr.internal_ddk_makefiles_dir[DdkSubmoduleInfo].kernel_module_deps.to_list()

    kernel_utils.check_kernel_build(
        [target.kernel_module_info for target in kernel_module_deps],
        ctx.attr.kernel_build.label,
        ctx.label,
    )
    _check_module_symvers_restore_path(kernel_module_deps, ctx.label)

    # Define where to build the external module (default to the package name)
    if ctx.attr.makefile:
        ext_mod_label = ctx.attr.makefile[0].label
    else:
        ext_mod_label = ctx.label
    ext_mod = paths.join(ext_mod_label.workspace_root, ext_mod_label.package)

    if not ext_mod:
        fail("""{label}: kernel_module must not be defined at the top-level package of the main repository.
                Move it to a sub-package, e.g. @{workspace_name}//{label_name}:{label_name}""".format(
            label = ctx.label,
            workspace_name = ctx.label.workspace_name,
            label_name = ctx.label.name,
        ))

    if ctx.files.makefile and ctx.file.internal_ddk_makefiles_dir:
        fail("{label}: must not define `makefile` for `ddk_module`".format(ctx.label))

    inputs = []
    inputs += ctx.files.makefile
    inputs += ctx.files.internal_ddk_makefiles_dir

    module_srcs = [target.files for target in ctx.attr.srcs]
    if not ctx.attr.internal_exclude_kernel_build_module_srcs:
        module_srcs.append(ctx.attr.kernel_build[KernelBuildExtModuleInfo].module_hdrs)
    module_srcs = depset(transitive = module_srcs)

    transitive_inputs = [module_srcs]
    for kernel_module_dep in kernel_module_deps:
        transitive_inputs.append(kernel_module_dep.kernel_module_setup_info.inputs)

    if ctx.attr.internal_ddk_makefiles_dir:
        transitive_inputs.append(ctx.attr.internal_ddk_makefiles_dir[DdkSubmoduleInfo].srcs)

    tools = [
        ctx.executable._check_declared_output_list,
        ctx.executable._search_and_cp_output,
        ctx.executable._print_gcno_mapping,
    ]
    transitive_tools = []

    modules_staging_dws = dws.make(ctx, "{}/staging".format(ctx.attr.name))
    kernel_uapi_headers_dws = dws.make(ctx, "{}/kernel-uapi-headers.tar.gz_staging".format(ctx.attr.name))
    outdir = modules_staging_dws.directory.dirname

    unstripped_dir = None
    if ctx.attr.kernel_build[KernelBuildExtModuleInfo].collect_unstripped_modules:
        unstripped_dir = ctx.actions.declare_directory("{name}/unstripped".format(name = ctx.label.name))

    output_files = [] + ctx.outputs.outs
    if ctx.attr.internal_ddk_makefiles_dir:
        for out in _get_implicit_outs(ctx):
            output_files.append(ctx.actions.declare_file("{}/{}".format(ctx.label.name, out)))

    # Original `outs` attribute of `kernel_module` macro.
    original_outs = []

    # apply basename to all of original_outs
    original_outs_base = []

    for out in output_files:
        # outdir includes target name at the end already. So short_name is the original
        # token in `outs` of `kernel_module` macro.
        # e.g. kernel_module(name = "foo", outs = ["bar"])
        #   => _kernel_module(name = "foo", outs = ["foo/bar"])
        #   => outdir = ".../foo"
        #      output_files = [File(".../foo/bar")]
        #   => short_name = "bar"
        short_name = out.path[len(outdir) + 1:]
        original_outs.append(short_name)
        original_outs_base.append(out.basename)

    all_module_names_file = ctx.actions.declare_file("{}/all_module_names.txt".format(ctx.label.name))
    ctx.actions.write(
        output = all_module_names_file,
        content = "\n".join(original_outs) + "\n",
    )
    inputs.append(all_module_names_file)

    module_symvers = ctx.actions.declare_file("{}/{}".format(ctx.attr.name, ctx.attr.internal_module_symvers_name))
    check_no_remaining = ctx.actions.declare_file("{name}/{name}.check_no_remaining".format(name = ctx.attr.name))
    command_outputs = [
        module_symvers,
        check_no_remaining,
    ]
    command_outputs += dws.files(modules_staging_dws)
    command_outputs += dws.files(kernel_uapi_headers_dws)
    if unstripped_dir:
        command_outputs.append(unstripped_dir)

    cache_dir_step = cache_dir.get_step(
        ctx = ctx,
        common_config_tags = ctx.attr.kernel_build[KernelEnvAttrInfo].common_config_tags,
        symlink_name = "module_{}".format(ctx.attr.name),
    )
    grab_cmd_step = get_grab_cmd_step(ctx, "${OUT_DIR}/${ext_mod_rel}")
    grab_gcno_step = get_grab_gcno_step(ctx, "${COMMON_OUT_DIR}", is_kernel_build = False)

    for step in (
        cache_dir_step,
        grab_cmd_step,
        grab_gcno_step,
    ):
        inputs += step.inputs
        command_outputs += step.outputs
        tools += step.tools

    # Determine the proper script to set up environment
    if ctx.attr.internal_ddk_config:
        setup_info = ctx.attr.internal_ddk_config[KernelSerializedEnvInfo]
    elif ctx.attr.generate_btf:
        # All outputs are required for BTF generation, including vmlinux image
        setup_info = ctx.attr.kernel_build[KernelBuildExtModuleInfo].mod_full_env
    else:
        setup_info = ctx.attr.kernel_build[KernelBuildExtModuleInfo].mod_min_env
    transitive_inputs.append(setup_info.inputs)
    transitive_tools.append(setup_info.tools)
    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = setup_info,
        restore_out_dir_cmd = cache_dir_step.cmd,
    )

    command += """
             # create dirs for modules
               mkdir -p {kernel_uapi_headers_dir}/usr
    """.format(
        kernel_uapi_headers_dir = kernel_uapi_headers_dws.directory.path,
    )
    for kernel_module_dep in kernel_module_deps:
        command += kernel_module_dep.kernel_module_setup_info.setup

    grab_unstripped_cmd = ""
    if unstripped_dir:
        grab_unstripped_cmd = """
            mkdir -p {unstripped_dir}
            {search_and_cp_output} --srcdir ${{OUT_DIR}}/${{ext_mod_rel}} --dstdir {unstripped_dir} {outs}
        """.format(
            search_and_cp_output = ctx.executable._search_and_cp_output.path,
            unstripped_dir = unstripped_dir.path,
            # Use basenames to flatten the unstripped directory, even though outs may contain items with slash.
            outs = " ".join(original_outs_base),
        )

    drop_modules_order_cmd = ""
    if ctx.attr.internal_drop_modules_order:
        drop_modules_order_cmd = """
            # Delete unnecessary modules.order.*, which will be re-generated by depmod.
              rm -f {modules_staging_dir}/lib/modules/*/extra/{ext_mod}/modules.order.*
        """.format(
            ext_mod = ext_mod,
            modules_staging_dir = modules_staging_dws.directory.path,
        )

    scmversion_ret = stamp.ext_mod_write_localversion(ctx, ext_mod)
    inputs += scmversion_ret.deps
    command += scmversion_ret.cmd

    if ctx.file.internal_ddk_makefiles_dir:
        command += """
             # Restore Makefile and Kbuild
               cp -r {ddk_makefiles}/* {ext_mod}/
             # Replace env var in cflags files
               find {ext_mod} -name '*.cflags' -exec sed -i'' -e 's:$(ROOT_DIR):'"${{ROOT_DIR}}"':g' {{}} \\+
        """.format(
            ddk_makefiles = ctx.file.internal_ddk_makefiles_dir.path,
            ext_mod = ext_mod,
        )

    module_strip_flag = "INSTALL_MOD_STRIP="
    if ctx.attr.kernel_build[KernelBuildExtModuleInfo].strip_modules:
        module_strip_flag += "1"

    modpost_warn = debug.modpost_warn(ctx)
    command += modpost_warn.cmd
    command_outputs += modpost_warn.outputs

    # Keep a record of the modules.order generated by `make`.
    modules_order = ctx.actions.declare_file("{}/modules.order".format(ctx.attr.name))
    command_outputs.append(modules_order)
    grab_modules_order_cmd = """
        # Backup modules.order files before optionally dropping them.
        cp -L -p {modules_staging_dir}/lib/modules/*/extra/{ext_mod}/modules.order.* {modules_order}
    """.format(
        ext_mod = ext_mod,
        modules_staging_dir = modules_staging_dws.directory.path,
        modules_order = modules_order.path,
    )

    make_filter = ""
    if not ctx.attr.generate_btf:
        # Filter out warnings if there is no need for BTF generation
        make_filter = " 2> >(sed '/Skipping BTF generation/d' >&2) "

    command += """
             # Set variables
               ext_mod_rel=$(realpath ${{ROOT_DIR}}/{ext_mod} --relative-to ${{KERNEL_DIR}})

             # Actual kernel module build
               make -C {ext_mod} ${{TOOL_ARGS}} M=${{ext_mod_rel}} O=${{OUT_DIR}} KERNEL_SRC=${{ROOT_DIR}}/${{KERNEL_DIR}} {make_filter} {make_redirect}
             # Install into staging directory
               make -C {ext_mod} ${{TOOL_ARGS}} DEPMOD=true M=${{ext_mod_rel}} \
                   O=${{OUT_DIR}} KERNEL_SRC=${{ROOT_DIR}}/${{KERNEL_DIR}}     \
                   INSTALL_MOD_PATH=$(realpath {modules_staging_dir})          \
                   INSTALL_MOD_DIR=extra/{ext_mod}                             \
                   KERNEL_UAPI_HEADERS_DIR=$(realpath {kernel_uapi_headers_dir}) \
                   INSTALL_HDR_PATH=$(realpath {kernel_uapi_headers_dir}/usr)  \
                   {module_strip_flag} modules_install

             # Check if there are remaining *.ko files
               remaining_ko_files=$({check_declared_output_list} \\
                    --declared $(cat {all_module_names_file}) \\
                    --actual $(cd {modules_staging_dir}/lib/modules/*/extra/{ext_mod} && find . -type f -name '*.ko' | sed 's:^[.]/::'))
               if [[ ${{remaining_ko_files}} ]]; then
                 echo "ERROR: The following kernel modules are built but not copied. Add these lines to the outs attribute of {label}:" >&2
                 for ko in ${{remaining_ko_files}}; do
                   echo '    "'"${{ko}}"'",' >&2
                 done
                 echo "Alternatively, install buildozer and execute:" >&2
                 echo "  $ buildozer 'add outs ${{remaining_ko_files}}' {label}" >&2
                 echo "See https://github.com/bazelbuild/buildtools/blob/master/buildozer/README.md for reference" >&2
                 exit 1
               fi
               touch {check_no_remaining}

             # Grab unstripped modules
               {grab_unstripped_cmd}
             # Grab *.gcno files
               {grab_gcno_step_cmd}
             # Grab *.cmd
               {grab_cmd_cmd}
             # Move Module.symvers
               rsync -aL ${{OUT_DIR}}/${{ext_mod_rel}}/Module.symvers {module_symvers}
             # Grab and then drop modules.order
               {grab_modules_order_cmd}
               {drop_modules_order_cmd}
               """.format(
        label = ctx.label,
        ext_mod = ext_mod,
        generate_btf = int(ctx.attr.generate_btf),
        make_filter = make_filter,
        make_redirect = modpost_warn.make_redirect,
        module_symvers = module_symvers.path,
        modules_staging_dir = modules_staging_dws.directory.path,
        outdir = outdir,
        kernel_uapi_headers_dir = kernel_uapi_headers_dws.directory.path,
        module_strip_flag = module_strip_flag,
        check_declared_output_list = ctx.executable._check_declared_output_list.path,
        all_module_names_file = all_module_names_file.path,
        grab_unstripped_cmd = grab_unstripped_cmd,
        check_no_remaining = check_no_remaining.path,
        grab_modules_order_cmd = grab_modules_order_cmd,
        drop_modules_order_cmd = drop_modules_order_cmd,
        grab_gcno_step_cmd = grab_gcno_step.cmd,
        grab_cmd_cmd = grab_cmd_step.cmd,
    )

    command += dws.record(modules_staging_dws)
    command += dws.record(kernel_uapi_headers_dws)

    # Unlike other rules (e.g. KernelBuild / ModulesPrepare), a DDK module must be executed
    # in a sandbox so that restoring the makefiles does not mutate the source tree. However,
    # we can't use linux-sandbox because --cache_dir is mounted as readonly. Hence, use
    # the weaker form processwrapper-sandbox instead.
    # See https://bazel.build/docs/sandboxing#sandboxing-strategies
    strategy = ""
    execution_requirements = None
    if ctx.attr._config_is_local[BuildSettingInfo].value:
        if ctx.file.internal_ddk_makefiles_dir:
            strategy = "ProcessWrapperSandbox"
        else:
            execution_requirements = kernel_utils.local_exec_requirements(ctx)

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "KernelModule" + strategy,
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = depset(tools, transitive = transitive_tools),
        outputs = command_outputs,
        command = command,
        progress_message = "Building external kernel module {}{}".format(
            ctx.attr.kernel_build[KernelEnvAttrInfo].progress_message_note,
            ctx.label,
        ),
        execution_requirements = execution_requirements,
    )

    # Additional outputs because of the value in outs. This is
    # [basename(out) for out in outs] - outs
    additional_declared_outputs = []
    for short_name, out in zip(original_outs, output_files):
        if "/" in short_name:
            additional_declared_outputs.append(ctx.actions.declare_file("{name}/{basename}".format(
                name = ctx.attr.name,
                basename = out.basename,
            )))
        original_outs_base.append(out.basename)
    cp_cmd_outputs = output_files + additional_declared_outputs

    if cp_cmd_outputs:
        hermetic_tools = hermetic_toolchain.get(ctx)
        command = hermetic_tools.setup + """
             # Copy files into place
               {search_and_cp_output} --srcdir {modules_staging_dir}/lib/modules/*/extra/{ext_mod}/ --dstdir {outdir} {outs}
        """.format(
            search_and_cp_output = ctx.executable._search_and_cp_output.path,
            modules_staging_dir = modules_staging_dws.directory.path,
            ext_mod = ext_mod,
            outdir = outdir,
            outs = " ".join(original_outs),
        )
        debug.print_scripts(ctx, command, what = "cp_outputs")
        ctx.actions.run_shell(
            mnemonic = "KernelModuleCpOutputs",
            inputs = [
                # We don't need structure_file here because we only care about files in the directory.
                modules_staging_dws.directory,
            ],
            tools = depset(
                [ctx.executable._search_and_cp_output],
                transitive = [hermetic_tools.deps],
            ),
            outputs = cp_cmd_outputs,
            command = command,
            progress_message = "Copying outputs {}".format(ctx.label),
        )

    module_symvers_restore_path = paths.join(ext_mod, ctx.attr.internal_module_symvers_name)
    setup = """
             # Use a new shell to avoid polluting variables
               (
             # Set variables
               # realpath requires the existence of ${{ROOT_DIR}}/{ext_mod}, which may not be the case for
               # _kernel_modules_install. Make that.
               mkdir -p ${{ROOT_DIR}}/{ext_mod}
               ext_mod_rel=$(realpath ${{ROOT_DIR}}/{ext_mod} --relative-to ${{KERNEL_DIR}})
             # Restore Modules.symvers
               mkdir -p $(dirname ${{COMMON_OUT_DIR}}/{module_symvers_restore_path})
               rsync -aL {module_symvers} ${{COMMON_OUT_DIR}}/{module_symvers_restore_path}
             # New shell ends
               )
    """.format(
        ext_mod = ext_mod,
        module_symvers = module_symvers.path,
        internal_module_symvers_name = ctx.attr.internal_module_symvers_name,
        module_symvers_restore_path = module_symvers_restore_path,
    )

    if ctx.attr.internal_ddk_makefiles_dir:
        ddk_headers_info = ctx.attr.internal_ddk_makefiles_dir[DdkHeadersInfo]
    else:
        ddk_headers_info = DdkHeadersInfo(
            files = depset(),
            includes = depset(),
            linux_includes = depset(),
        )

    if ctx.attr.internal_ddk_config:
        ddk_config_info = ctx.attr.internal_ddk_config[DdkConfigInfo]
    else:
        ddk_config_info = DdkConfigInfo(kconfig = depset(), defconfig = depset())

    # Only declare outputs in the "outs" list. For additional outputs that this rule created,
    # the label is available, but this rule doesn't explicitly return it in the info.
    # Also add check_no_remaining in the list of default outputs so that, when
    # outs is empty, the KernelModule action is still executed, and so
    # is check_declared_output_list.
    return [
        # Sync list of infos with kernel_module_group.
        DefaultInfo(
            files = depset(output_files + [check_no_remaining, module_symvers] + grab_gcno_step.outputs),
            # For kernel_module_test
            runfiles = ctx.runfiles(files = output_files),
        ),
        KernelModuleSetupInfo(
            inputs = depset([module_symvers]),
            setup = setup,
        ),
        KernelModuleInfo(
            kernel_build_infos = kernel_utils.create_kernel_module_kernel_build_info(ctx.attr.kernel_build),
            modules_staging_dws_depset = depset([modules_staging_dws]),
            kernel_uapi_headers_dws_depset = depset([kernel_uapi_headers_dws]),
            files = depset(output_files),
            packages = depset([ext_mod]),
            label = ctx.label,
            modules_order = depset([modules_order]),
        ),
        KernelUnstrippedModulesInfo(
            directories = depset([unstripped_dir], order = "postorder"),
        ),
        ModuleSymversInfo(
            # path/to/package/target_name/Module.symvers -> path/to/package/Module.symvers;
            # path/to/package/target_name/target_name_Module.symvers -> path/to/package/target_name_Module.symvers;
            # This is similar to ${{OUT_DIR}}/${{ext_mod_rel}}
            # It is needed to remove the `target_name` because we declare_file({name}/{internal_module_symvers_name}) above.
            restore_paths = depset([module_symvers_restore_path]),
        ),
        ddk_headers_info,
        ddk_config_info,
        GcovInfo(
            gcno_mapping = grab_gcno_step.gcno_mapping,
            gcno_dir = grab_gcno_step.gcno_dir,
        ),
        KernelCmdsInfo(
            srcs = module_srcs,
            directories = depset([grab_cmd_step.cmd_dir]),
        ),
    ]

def _kernel_module_additional_attrs():
    return cache_dir.attrs() | stamp.ext_mod_attrs()

_kernel_module = rule(
    implementation = _kernel_module_impl,
    doc = """
""",
    attrs = {
        "srcs": attr.label_list(
            mandatory = True,
            allow_files = True,
        ),
        "makefile": attr.label_list(
            allow_files = True,
            doc = "Used internally. The makefile for this module.",
        ),
        "internal_ddk_makefiles_dir": attr.label(
            allow_single_file = True,  # A single directory
            doc = "A `makefiles` target that denotes a list of makefiles to restore",
        ),
        "internal_module_symvers_name": attr.string(default = "Module.symvers"),
        "internal_drop_modules_order": attr.bool(),
        "internal_exclude_kernel_build_module_srcs": attr.bool(),
        "internal_ddk_config": attr.label(providers = [
            KernelSerializedEnvInfo,
            DdkConfigInfo,
        ]),
        "generate_btf": attr.bool(
            default = False,
            doc = "See [kernel_module.generate_btf](#kernel_module-generate_btf)",
        ),
        "kernel_build": attr.label(
            mandatory = True,
            providers = [KernelBuildExtModuleInfo],
        ),
        "deps": attr.label_list(),
        # Not output_list because it is not a list of labels. The list of
        # output labels are inferred from name and outs.
        "outs": attr.output_list(),
        "_search_and_cp_output": attr.label(
            default = Label("//build/kernel/kleaf:search_and_cp_output"),
            cfg = "exec",
            executable = True,
            doc = "Label referring to the script to process outputs",
        ),
        "_check_declared_output_list": attr.label(
            default = Label("//build/kernel/kleaf:check_declared_output_list"),
            cfg = "exec",
            executable = True,
        ),
        "_config_is_stamp": attr.label(default = "//build/kernel/kleaf:config_stamp"),
        "_preserve_cmd": attr.label(default = "//build/kernel/kleaf/impl:preserve_cmd"),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_debug_modpost_warn": attr.label(default = "//build/kernel/kleaf:debug_modpost_warn"),
    } | _kernel_module_additional_attrs() | gcov_attrs(),
    toolchains = [hermetic_toolchain.type],
)

def _kernel_module_set_defaults(kwargs):
    """Set default values for `_kernel_module` that can't be specified in `attr.*(default=...)` in rule()."""
    if kwargs.get("makefile") == None and kwargs.get("internal_ddk_makefiles_dir") == None:
        kwargs["makefile"] = native.glob(["Makefile"])

    if kwargs.get("outs") == None:
        kwargs["outs"] = ["{}.ko".format(kwargs["name"])]

    if kwargs.get("srcs") == None:
        kwargs["srcs"] = native.glob([
            "**/*.c",
            "**/*.h",
            "**/Kbuild",
            "**/Makefile",
        ])

    return kwargs
