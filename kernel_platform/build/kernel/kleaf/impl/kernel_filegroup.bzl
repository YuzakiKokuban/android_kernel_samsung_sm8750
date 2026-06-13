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

"""A target that mimics [`kernel_build`](#kernel_build) from a list of prebuilt files."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load(
    ":common_providers.bzl",
    "GcovInfo",
    "KernelBuildAbiInfo",
    "KernelBuildExtModuleInfo",
    "KernelBuildInTreeModulesInfo",
    "KernelBuildMixedTreeInfo",
    "KernelBuildUapiInfo",
    "KernelBuildUnameInfo",
    "KernelEnvAttrInfo",
    "KernelImagesInfo",
    "KernelSerializedEnvInfo",
    "KernelToolchainInfo",
    "KernelUnstrippedModulesInfo",
)
load(
    ":constants.bzl",
    "MODULES_STAGING_ARCHIVE",
    "UNSTRIPPED_MODULES_ARCHIVE",
)
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":kernel_build.bzl", "create_serialized_env_info")
load(":kernel_config.bzl", "get_config_setup_command")
load(":kernel_config_settings.bzl", "kernel_config_settings")
load(":kernel_env.bzl", "get_env_info_setup_command")
load(":kernel_toolchains_utils.bzl", "kernel_toolchains_utils")
load(":modules_prepare.bzl", "modules_prepare_setup_command")
load(
    ":utils.bzl",
    "utils",
)

visibility("//build/kernel/kleaf/...")

def _get_mixed_tree_files(target):
    if KernelBuildMixedTreeInfo in target:
        return target[KernelBuildMixedTreeInfo].files
    return target.files

def _get_toolchain_version_info(ctx):
    return KernelToolchainInfo(
        toolchain_version = kernel_toolchains_utils.get(ctx).compiler_version,
    )

def _get_kernel_release(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    kernel_release = ctx.file.kernel_release
    if kernel_release:
        return kernel_release

    # TODO(b/291918087): Delete legacy code path once users are not present.
    gki_info = utils.find_file(
        name = "gki-info.txt",
        files = ctx.files.gki_artifacts,
        what = "{} gki_artifacts".format(ctx.label),
        required = True,
    )
    kernel_release = ctx.actions.declare_file("{}/kernel.release".format(ctx.label.name))
    command = hermetic_tools.setup + """
        kernel_release=$(cat {gki_info} | sed -nE 's/^kernel_release=(.*)$/\\1/p')
        if [[ -z "${{kernel_release}}" ]]; then
            echo "ERROR: Unable to determine kernel_release from {gki_info}" >&2
            exit 1
        fi
        echo "${{kernel_release}}" > {kernel_release_file}
    """.format(
        gki_info = gki_info.path,
        kernel_release_file = kernel_release.path,
    )
    debug.print_scripts(ctx, command, what = "kernel.release")
    ctx.actions.run_shell(
        command = command,
        inputs = [gki_info],
        outputs = [kernel_release],
        tools = hermetic_tools.deps,
        progress_message = "Extracting kernel.release {}".format(ctx.label),
        mnemonic = "KernelFilegroupKernelRelease",
    )
    return kernel_release

def _get_config_env(ctx):
    """Returns a KernelSerializedEnvInfo analogous to that returned by kernel_config().
    """

    if not ctx.file.config_out_dir or not ctx.file.env_setup_script:
        return None

    hermetic_tools = hermetic_toolchain.get(ctx)
    toolchains = kernel_toolchains_utils.get(ctx)

    env_setup_command = """
        KLEAF_REPO_WORKSPACE_ROOT={kleaf_repo_workspace_root}
        KLEAF_HERMETIC_BASE={hermetic_base}
    """.format(
        kleaf_repo_workspace_root = Label(":kernel_filegroup.bzl").workspace_root,
        hermetic_base = hermetic_tools.internal_hermetic_base,
    )
    env_setup_command += get_env_info_setup_command(
        hermetic_tools_setup = hermetic_tools.setup,
        build_utils_sh = ctx.file._build_utils_sh,
        env_setup_script = ctx.file.env_setup_script,
    )

    config_env_setup_command = get_config_setup_command(
        env_setup_command = env_setup_command,
        out_dir = ctx.file.config_out_dir,
        extra_restore_outputs_cmd = "",
    )

    config_env_setup_script = ctx.actions.declare_file(
        "{name}/{name}_config_setup.sh".format(name = ctx.attr.name),
    )

    ctx.actions.write(
        output = config_env_setup_script,
        content = config_env_setup_command,
    )
    config_env = KernelSerializedEnvInfo(
        setup_script = config_env_setup_script,
        inputs = depset([
            config_env_setup_script,
            ctx.file.env_setup_script,
            ctx.version_file,
        ], transitive = [target.files for target in ctx.attr.config_out_dir_files]),
        tools = depset([
            ctx.file._build_utils_sh,
        ], transitive = [
            hermetic_tools.deps,
            toolchains.all_files,
        ]),
    )
    return config_env

def _get_serialized_env(ctx, config_env, outs_mapping, internal_outs_mapping):
    """Returns `KernelSerializedEnvInfo` analogous to the one returned by kernel_build().

    Unlike kernel_build(), this does not include implicit_outs, because
    they are dropped from the dist artifacts.
    """

    if not config_env:
        return None

    return create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_setup.sh".format(name = ctx.attr.name),
        pre_info = config_env,
        outputs = outs_mapping | internal_outs_mapping,
        fake_system_map = False,
        # kernel_filegroup does not have base_kernel, so no need to restore kbuild_mixed_tree
        extra_restore_outputs_cmd = "",
        extra_inputs = depset(),
    )

def _get_ddk_config_env(ctx, config_env):
    """Returns `KernelBuildExtModuleInfo.ddk_config_env`."""

    if not config_env:
        return None

    if not ctx.file.module_env_archive:
        return None

    extra_restore_outputs_cmd = """
        # Restore module sources
        {check_sandbox_cmd}
        tar xf {module_env_archive} -C ${{KLEAF_REPO_DIR}}
    """.format(
        module_env_archive = ctx.file.module_env_archive.path,
        check_sandbox_cmd = utils.get_check_sandbox_cmd(),
    )

    return create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_ddk_config_setup.sh".format(name = ctx.attr.name),
        pre_info = config_env,
        outputs = {},
        fake_system_map = False,
        extra_restore_outputs_cmd = extra_restore_outputs_cmd,
        extra_inputs = depset([ctx.file.module_env_archive]),
    )

def _get_modules_prepare_env(ctx, ddk_config_env):
    """Returns a KernelSerializedEnvInfo analogous to that returned by modules_prepare().

    Unlike modules_prepare(), this also incorporates ddk_config_env so that
    module_env_archive is extracted.
    """

    if not ddk_config_env:
        return None

    if not ctx.file.modules_prepare_archive:
        return None

    modules_prepare_setup = modules_prepare_setup_command(
        config_setup_script = ddk_config_env.setup_script,
        modules_prepare_outdir_tar_gz = ctx.file.modules_prepare_archive,
    )

    module_prepare_env_setup_script = ctx.actions.declare_file(
        "{name}/{name}_modules_prepare_setup.sh".format(name = ctx.attr.name),
    )
    ctx.actions.write(
        output = module_prepare_env_setup_script,
        content = modules_prepare_setup,
    )
    return KernelSerializedEnvInfo(
        setup_script = module_prepare_env_setup_script,
        inputs = depset([
            module_prepare_env_setup_script,
            ctx.file.modules_prepare_archive,
        ], transitive = [ddk_config_env.inputs]),
        tools = ddk_config_env.tools,
    )

def _expect_single_file(target, what):
    """Returns a single file from the given Target."""
    list_of_files = target.files.to_list()
    if len(list_of_files) != 1:
        fail("{} expects exactly one file, but got {}".format(what, list_of_files))
    return list_of_files[0]

def _get_mod_envs(ctx, modules_prepare_env, outs_mapping, internal_outs_mapping):
    """Returns partial `KernelBuildExtModuleInfo` with mod_*_env fields."""
    if modules_prepare_env == None:
        return KernelBuildExtModuleInfo(
            mod_min_env = None,
            mod_full_env = None,
            modinst_env = None,
        )

    mod_min_env = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_mod_min_setup.sh".format(name = ctx.attr.name),
        pre_info = modules_prepare_env,
        outputs = internal_outs_mapping,
        fake_system_map = True,
        extra_restore_outputs_cmd = "",
        extra_inputs = depset(),
    )

    mod_full_env = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_mod_full_setup.sh".format(name = ctx.attr.name),
        pre_info = modules_prepare_env,
        outputs = outs_mapping | internal_outs_mapping,
        fake_system_map = False,
        # kernel_filegroup does not have base_kernel, so no need to restore kbuild_mixed_tree
        extra_restore_outputs_cmd = "",
        extra_inputs = depset(),
    )

    return KernelBuildExtModuleInfo(
        mod_min_env = mod_min_env,
        mod_full_env = mod_full_env,
        modinst_env = mod_full_env,
    )

def _kernel_filegroup_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)

    all_deps = ctx.files.srcs + ctx.files.deps

    # {File(...): "vmlinux", ...}
    outs_mapping = {
        _expect_single_file(target, what = "{}: outs".format(ctx.label)): relpath
        for target, relpath in ctx.attr.outs.items()
    }

    # {File(...): "Module.symvers", ...}
    internal_outs_mapping = {
        _expect_single_file(target, what = "{}: internal_outs".format(ctx.label)): relpath
        for target, relpath in ctx.attr.internal_outs.items()
    }

    config_env = _get_config_env(ctx)
    serialized_env = _get_serialized_env(
        ctx = ctx,
        config_env = config_env,
        outs_mapping = outs_mapping,
        internal_outs_mapping = internal_outs_mapping,
    )
    ddk_config_env = _get_ddk_config_env(ctx, config_env)
    modules_prepare_env = _get_modules_prepare_env(ctx, ddk_config_env)
    mod_envs = _get_mod_envs(
        ctx = ctx,
        modules_prepare_env = modules_prepare_env,
        outs_mapping = outs_mapping,
        internal_outs_mapping = internal_outs_mapping,
    )

    kernel_module_dev_info = KernelBuildExtModuleInfo(
        modules_staging_archive = utils.find_file(MODULES_STAGING_ARCHIVE, all_deps, what = ctx.label),
        # Building kernel_module (excluding ddk_module) on top of kernel_filegroup is unsupported.
        # module_hdrs = None,
        ddk_config_env = ddk_config_env,
        mod_min_env = mod_envs.mod_min_env,
        mod_full_env = mod_envs.mod_full_env,
        modinst_env = mod_envs.modinst_env,
        collect_unstripped_modules = ctx.attr.collect_unstripped_modules,
        ddk_module_defconfig_fragments = depset(transitive = [
            target.files
            for target in ctx.attr.ddk_module_defconfig_fragments
        ]),
        strip_modules = ctx.attr.strip_modules,
    )

    kernel_uapi_depsets = []
    if ctx.attr.kernel_uapi_headers:
        kernel_uapi_depsets.append(ctx.attr.kernel_uapi_headers.files)
    uapi_info = KernelBuildUapiInfo(
        kernel_uapi_headers = depset(transitive = kernel_uapi_depsets, order = "postorder"),
    )

    unstripped_modules_info = None
    for target in ctx.attr.srcs:
        if KernelUnstrippedModulesInfo in target:
            unstripped_modules_info = target[KernelUnstrippedModulesInfo]
            break
    if unstripped_modules_info == None:
        # Reverse of kernel_unstripped_modules_archive
        unstripped_modules_archive = utils.find_file(UNSTRIPPED_MODULES_ARCHIVE, all_deps, what = ctx.label, required = True)
        unstripped_dir = ctx.actions.declare_directory("{}/unstripped".format(ctx.label.name))
        command = hermetic_tools.setup + """
            tar xf {unstripped_modules_archive} -C $(dirname {unstripped_dir}) $(basename {unstripped_dir})
        """.format(
            unstripped_modules_archive = unstripped_modules_archive.path,
            unstripped_dir = unstripped_dir.path,
        )
        debug.print_scripts(ctx, command, what = "unstripped_modules_archive")
        ctx.actions.run_shell(
            command = command,
            inputs = [
                unstripped_modules_archive,
            ],
            outputs = [unstripped_dir],
            tools = hermetic_tools.deps,
            progress_message = "Extracting unstripped_modules_archive {}".format(ctx.label),
            mnemonic = "KernelFilegroupUnstrippedModulesArchive",
        )
        unstripped_modules_info = KernelUnstrippedModulesInfo(
            directories = depset([unstripped_dir], order = "postorder"),
        )

    protected_modules_list = None
    if ctx.files.protected_modules_list:
        if len(ctx.files.protected_modules_list) != 1:
            fail("{}: protected_modules_list {} produces multiple files, expected 0 or 1".format(
                ctx.label,
                ctx.attr.protected_modules_list,
            ))
        protected_modules_list = ctx.files.protected_modules_list[0]

    abi_info = KernelBuildAbiInfo(
        src_protected_modules_list = protected_modules_list,
        modules_staging_archive = utils.find_file(MODULES_STAGING_ARCHIVE, all_deps, what = ctx.label),
    )
    in_tree_modules_info = KernelBuildInTreeModulesInfo(all_module_names = ctx.attr.all_module_names)

    images_info = KernelImagesInfo(
        base_kernel_label = None,
        outs = depset(transitive = [target.files for target in ctx.attr.outs]),
        base_kernel_files = depset(),
    )
    gcov_info = GcovInfo(gcno_mapping = None, gcno_dir = None)

    # kernel_filegroup does not have any defconfig_fragments because the .config is fixed from prebuilts.
    config_tags_out = kernel_config_settings.kernel_env_get_config_tags(
        ctx = ctx,
        mnemonic_prefix = "KernelFilegroup",
        defconfig_fragments = [],
    )
    progress_message_note = kernel_config_settings.get_progress_message_note(
        ctx,
        defconfig_fragments = [],
    )
    kernel_env_attr_info = KernelEnvAttrInfo(
        common_config_tags = config_tags_out.common,
        progress_message_note = progress_message_note,
    )

    srcs_depset = depset(transitive = [target.files for target in ctx.attr.srcs])
    mixed_tree_files = depset(transitive = [_get_mixed_tree_files(target) for target in ctx.attr.srcs])
    kernel_release = _get_kernel_release(ctx)

    infos = [
        DefaultInfo(files = srcs_depset),
        KernelBuildMixedTreeInfo(files = mixed_tree_files),
        KernelBuildUnameInfo(kernel_release = kernel_release),
        kernel_module_dev_info,
        uapi_info,
        unstripped_modules_info,
        abi_info,
        in_tree_modules_info,
        images_info,
        kernel_env_attr_info,
        gcov_info,
        _get_toolchain_version_info(ctx),
    ]
    if serialized_env:
        infos.append(serialized_env)
    return infos

def _kernel_filegroup_additional_attrs():
    return dicts.add(
        kernel_config_settings.of_kernel_env(),
        kernel_toolchains_utils.attrs(),
    )

kernel_filegroup = rule(
    implementation = _kernel_filegroup_impl,
    doc = """**EXPERIMENTAL.** The API of `kernel_filegroup` rapidly changes and
is not backwards compatible with older builds. The usage of `kernel_filegroup`
is limited to the implementation detail of Kleaf (in particular,
[`define_common_kernels`](#define_common_kernels)). Do not use
`kernel_filegroup` directly. See `download_prebuilt.md` for details.

Specify a list of kernel prebuilts.

This is similar to [`filegroup`](https://docs.bazel.build/versions/main/be/general.html#filegroup)
that gives a convenient name to a collection of targets, which can be referenced from other rules.

It can be used in the `base_kernel` attribute of a [`kernel_build`](#kernel_build).
""",
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            doc = """The list of labels that are members of this file group.

This usually contains a list of prebuilts, e.g. `vmlinux`, `Image.lz4`, `kernel-headers.tar.gz`,
etc.

Not to be confused with [`kernel_srcs`](#kernel_filegroup-kernel_srcs).""",
        ),
        "deps": attr.label_list(
            allow_files = True,
            doc = """A list of additional labels that participates in implementing the providers.

This usually contains a list of prebuilts.

Unlike srcs, these labels are NOT added to the [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html)""",
        ),
        "kernel_uapi_headers": attr.label(
            allow_files = True,
            doc = """The label pointing to `kernel-uapi-headers.tar.gz`.

This attribute should be set to the `kernel-uapi-headers.tar.gz` artifact built by the
[`kernel_build`](#kernel_build) macro if the `kernel_filegroup` rule were a `kernel_build`.

Setting this attribute allows [`merged_kernel_uapi_headers`](#merged_kernel_uapi_headers) to
work properly when this `kernel_filegroup` is set to the `base_kernel`.

For example:
```
kernel_filegroup(
    name = "kernel_aarch64_prebuilts",
    srcs = [
        "vmlinux",
        # ...
    ],
    kernel_uapi_headers = "kernel-uapi-headers.tar.gz",
)

kernel_build(
    name = "tuna",
    base_kernel = ":kernel_aarch64_prebuilts",
    # ...
)

merged_kernel_uapi_headers(
    name = "tuna_merged_kernel_uapi_headers",
    kernel_build = "tuna",
    # ...
)
```
""",
        ),
        "collect_unstripped_modules": attr.bool(
            default = True,
            doc = """See [`kernel_build.collect_unstripped_modules`](#kernel_build-collect_unstripped_modules).

Unlike `kernel_build`, this has default value `True` because
[`kernel_abi`](#kernel_abi) sets
[`define_abi_targets`](#kernel_abi-define_abi_targets) to `True` by
default, which in turn sets `collect_unstripped_modules` to `True` by default.
""",
        ),
        "strip_modules": attr.bool(
            doc = """See [`kernel_build.strip_modules`](#kernel_build-strip_modules).""",
        ),
        "all_module_names": attr.string_list(
            doc = """`module_outs` and `module_implicit_outs` of the original
                [`kernel_build`](#kernel_build) target.""",
        ),
        "images": attr.label(
            allow_files = True,
            doc = """A label providing files similar to a [`kernel_images`](#kernel_images) target.""",
        ),
        "protected_modules_list": attr.label(allow_files = True),
        "gki_artifacts": attr.label(
            allow_files = True,
            doc = """A list of files that were built from the [`gki_artifacts`](#gki_artifacts) target.
                The `gki-info.txt` file should be part of that list.

                If `kernel_release` is set, this attribute has no effect.
            """,
        ),
        "kernel_release": attr.label(
            allow_single_file = True,
            doc = "A file providing the kernel release string. This is preferred over `gki_artifacts`.",
        ),
        "config_out_dir_files": attr.label_list(
            doc = "Files in `config_out_dir`",
            allow_files = True,
        ),
        "config_out_dir": attr.label(
            allow_single_file = True,
            doc = "Directory to support `kernel_config`",
        ),
        "env_setup_script": attr.label(
            allow_single_file = True,
            doc = "Setup script from `kernel_env`",
        ),
        "modules_prepare_archive": attr.label(
            allow_single_file = True,
            doc = "Archive from `modules_prepare`",
        ),
        "module_env_archive": attr.label(
            allow_single_file = True,
            doc = """Archive from `kernel_build.pack_module_env` that contains
                necessary files to build external modules.""",
        ),
        "outs": attr.label_keyed_string_dict(
            allow_files = True,
            doc = "Keys: from `_kernel_build.outs`. Values: path under `$OUT_DIR`.",
        ),
        "internal_outs": attr.label_keyed_string_dict(
            allow_files = True,
            doc = "Keys: from `_kernel_build.internal_outs`. Values: path under `$OUT_DIR`.",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_cache_dir_config_tags": attr.label(
            default = "//build/kernel/kleaf/impl:cache_dir_config_tags",
            executable = True,
            cfg = "exec",
        ),
        "_build_utils_sh": attr.label(
            allow_single_file = True,
            default = Label("//build/kernel:build_utils"),
            cfg = "exec",
        ),
        "ddk_module_defconfig_fragments": attr.label_list(
            doc = "Additional defconfig fragments for dependant DDK modules.",
            allow_empty = True,
            allow_files = True,
        ),
    } | _kernel_filegroup_additional_attrs(),
    toolchains = [hermetic_toolchain.type],
)
