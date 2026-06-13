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

"""Providers that are provided by multiple rules in different extensions."""

visibility("//build/kernel/kleaf/...")

KernelCmdsInfo = provider(
    doc = """Provides a directory of `.cmd` files.""",
    fields = {
        "srcs": """A [depset](https://bazel.build/extending/depsets) of sources to build
            the original target.""",
        "directories": """A [depset](https://bazel.build/extending/depsets) of directories
                          containing the `.cmd` files""",
    },
)

KernelBuildConfigInfo = provider(
    doc = """Provides build config dependencies""",
    fields = {
        "deps": "additional dependencies",
    },
)

KernelEnvInfo = provider(
    doc = """Describe a generic environment setup with some dependencies and a setup script.""",
    fields = {
        "inputs": """A [depset](https://bazel.build/extending/depsets) of inputs associated with
            the target platform.""",
        "tools": """A [depset](https://bazel.build/extending/depsets) of tools associated with
            the execution platform.""",
        "setup": "setup script to initialize the environment",
        "run_env": """Optional `KernelEnvInfo` to initialize the environment for `bazel run`.

For `kernel_env`, the script only provides a bare-minimum environment after `source build.config`,
without actually modifying any variables suitable for a proper kernel build.
""",
    },
)

KernelEnvMakeGoalsInfo = provider(
    doc = "Describe the targets for the current build.",
    fields = {
        "make_goals": "A list of strings defining targets for the kernel build.",
    },
)

KernelPlatformToolchainInfo = provider(
    doc = """Provides toolchain information of a single platform (target or execution).""",
    fields = {
        "compiler_version": "A string representing compiler version",
        "toolchain_id": "A string representing toolchain ID for debugging purposes",
        "all_files": "A [depset](https://bazel.build/extending/depsets) of all files of the toolchain",
        "cflags": "flags for C compilation",
        "ldflags": "flags for C linking",
        "bin_path": "`PATH` relative to execroot.",
    },
)

KernelToolchainInfo = provider(
    doc = "Provides a single toolchain version.",
    fields = {
        "toolchain_version": "The toolchain version",
    },
)

KernelEnvToolchainsInfo = provider(
    doc = """Provides resolved toolchains information to `kernel_env`.""",
    fields = {
        "compiler_version": "A string representing compiler version",
        "all_files": "A [depset](https://bazel.build/extending/depsets) of all files of all toolchains",
        "target_arch": "arch of target platform",
        "setup_env_var_cmd": "A command to set up environment variables",
    },
)

KernelSerializedEnvInfo = provider(
    doc = """Like `KernelEnvInfo` but also restores artifacts.

It is expected to be created like the following:

```
setup_script = ctx.actions.declare_file("{}/setup.sh".format(ctx.attr.name))
ctx.actions.write(
    output = setup_script,
    content = \"""
        {pre_setup}
        {eval_restore_out_dir_cmd}
    \""".format(
        pre_setup = pre_setup, # sets up hermetic toolchain and environment variables
        eval_restore_out_dir_cmd = kernel_utils.eval_restore_out_dir_cmd(),
    )
)

serialized_env_info = KernelSerializedEnvInfo(
    setup_script = setup_script,
    tools = ...,
    inputs = depset([setup_script], ...),
)
```

It is expected to use these infos in the following way:

```
command = \"""
    KLEAF_RESTORE_OUT_DIR_CMD="{restore_out_dir_cmd}"
    . {setup_script}
\""".format(
    restore_out_dir_cmd = cache_dir_step.cmd, # or utils.get_check_sandbox_cmd(),
    setup_script = ctx.attr.dep[KernelSerializedEnvInfo].setup_script.path,
)
```
""",
    fields = {
        "setup_script": "A file containing the setup script.",
        "inputs": """A [depset](https://bazel.build/extending/depsets) containing inputs used
                   by `setup_script`. Note that dependencies of `restore_out_dir_cmd` is not
                   included. `inputs` are compiled against the target platform.

                   For convenience for the caller / user of the info, `inputs` should include
                   `setup_script`.
                   """,
        "tools": """A [depset](https://bazel.build/extending/depsets) containing tools used
                   by `setup_script`. Note that dependencies of `restore_out_dir_cmd` is not
                   included. `tools` are compiled against the execution platform.""",
    },
)

KernelBuildOriginalEnvInfo = provider(
    doc = """For `kernel_build` to expose `KernelEnvInfo` from `kernel_env`.""",
    fields = {
        "env_info": "`KernelEnvInfo` from `kernel_env`",
    },
)

KernelEnvAttrInfo = provider(
    doc = "Provide attributes of `kernel_env`.",
    fields = {
        "kbuild_symtypes": "`KBUILD_SYMTYPES`, after resolving `--kbuild_symtypes` and the static value.",
        "progress_message_note": """A note in the progress message that differentiates multiple
            instances of the same action due to different configs.""",
        "common_config_tags": "A File denoting the configurations that are useful to isolate `OUT_DIR`.",
    },
)

KernelBuildInfo = provider(
    doc = """Generic information provided by a `kernel_build`.""",
    fields = {
        "out_dir_kernel_headers_tar": "Archive containing headers in `OUT_DIR`",
        "outs": "A list of File object corresponding to the `outs` attribute (excluding `module_outs`, `implicit_outs` and `internal_outs`)",
        "base_kernel_files": """A [depset](https://bazel.build/extending/depsets) containing
            [Default outputs](https://docs.bazel.build/versions/main/skylark/rules.html#default-outputs)
            of the rule specified by `base_kernel`""",
        "interceptor_output": "`interceptor` log. See [`interceptor`](https://android.googlesource.com/kernel/tools/interceptor/) project.",
        "compile_commands_with_vars": "A file that can be transformed into `compile_commands.json`.",
        "compile_commands_out_dir": "A subset of `$OUT_DIR` for `compile_commands.json`.",
        "kernel_release": "The file `kernel.release`.",
    },
)

KernelBuildExtModuleInfo = provider(
    doc = "A provider that specifies the expectations of a `_kernel_module` (an external module) or a `kernel_modules_install` from its `kernel_build` attribute.",
    fields = {
        "modules_staging_archive": "Archive containing staging kernel modules. " +
                                   "Does not contain the lib/modules/* suffix.",
        "module_hdrs": "A [depset](https://bazel.build/extending/depsets) containing headers for this `kernel_build` for building external modules",
        "ddk_config_env": "`KernelSerializedEnvInfo` for configuring DDK modules (excl. legacy `kernel_module`).",
        "ddk_module_defconfig_fragments": "A [depset](https://bazel.build/extending/depsets) containing additional defconfig fragments for DDK modules.",
        "mod_min_env": "`KernelSerializedEnvInfo` for building external modules, including minimal needed `kernel_build` outputs.",
        "mod_full_env": "`KernelSerializedEnvInfo` for building external modules, including all `kernel_build` outputs.",
        "modinst_env": "`KernelSerializedEnvInfo` for running `modules_install`.",
        "collect_unstripped_modules": "Whether an external [`kernel_module`](kernel.md#kernel_module) building against this [`kernel_build`](kernel.md#kernel_build) should provide unstripped ones for debugging.",
        "strip_modules": "Whether debug information for distributed modules is stripped",
    },
)

KernelBuildUapiInfo = provider(
    doc = "A provider that specifies the expecation of a `merged_uapi_headers` rule from its `kernel_build` attribute.",
    fields = {
        "kernel_uapi_headers": """A [depset](https://bazel.build/extending/depsets) containing
            kernel UAPI headers archive.

            Order matters; earlier elements in the traverse order has higher priority. Hence,
            this depset must have `order` argument specified.
            """,
    },
)

KernelBuildAbiInfo = provider(
    doc = "A provider that specifies ABI-related information of a [`kernel_build`](kernel.md#kernel_build).",
    fields = {
        "trim_nonlisted_kmi": "Value of `trim_nonlisted_kmi` in [`kernel_build()`](kernel.md#kernel_build).",
        "combined_abi_symbollist": "The **combined** `abi_symbollist` file from the `_kmi_symbol_list` rule, consist of the source `kmi_symbol_list` and `additional_kmi_symbol_lists`.",
        "modules_staging_archive": "Archive containing staging kernel modules. ",
        "base_modules_staging_archive": "Archive containing staging kernel modules of the base kernel",
        "src_kmi_symbol_list": """Source file for `kmi_symbol_list` that points to the symbol list
                                  to be updated by `--update_symbol_list`""",
        "src_protected_exports_list": """Source file for protected symbols which are restricted from being exported by unsigned modules to be updated by `--update_protected_exports`""",
        "src_protected_modules_list": """Source file with list of protected modules whose exports are being protected and needs to be updated by `--update_protected_exports`""",
        "kmi_strict_mode_out": "A [`File`](https://bazel.build/rules/lib/File) to force kmi_strict_mode check.",
    },
)

KernelBuildInTreeModulesInfo = provider(
    doc = """A provider that specifies the expectations of a [`kernel_build`](kernel.md#kernel_build) on its
[`base_kernel`](kernel.md#kernel_build-base_kernel) for the list of in-tree modules in the `base_kernel`.""",
    fields = {
        "all_module_names": """`[kernel_build.module_outs]`(kernel.md#kernel_build-module_outs)
            and `[kernel_build.module_implicit_outs]`(kernel.md#kernel_build-module_implicit_outs).
        """,
    },
)

KernelBuildMixedTreeInfo = provider(
    doc = """A provider that specifies the expectations of a [`kernel_build`](kernel.md#kernel_build) on its
[`base_kernel`](kernel.md#kernel_build-base_kernel) for constructing `KBUILD_MIXED_TREE`.""",
    fields = {
        "files": """A [depset](https://bazel.build/extending/depsets) containing the list of
files required to build `KBUILD_MIXED_TREE` for the device kernel.""",
    },
)

KernelBuildUnameInfo = provider(
    doc = """A provider providing `kernel.release` of a `kernel_build`.""",
    fields = {
        "kernel_release": "The file `kernel.release`.",
    },
)

KernelBuildFilegroupDeclInfo = provider(
    doc = """A provider providing information of a `kernel_build` to generate `kernel_filegroup`
        declaration.""",
    fields = {
        "filegroup_srcs": """[depset](https://bazel.build/extending/depsets) of
            [`File`](https://bazel.build/rules/lib/File)s that the
            `kernel_filegroup` should return as default outputs.""",
        "all_module_names": """
            `[kernel_build.module_outs]`(kernel.md#kernel_build-module_outs) and
            `[kernel_build.module_implicit_outs]`(kernel.md#kernel_build-module_implicit_outs).""",
        "modules_staging_archive": "Archive containing staging kernel modules. ",
        "toolchain_version": "The toolchain version",
        "kernel_release": "The file `kernel.release`.",
        "modules_prepare_archive": """Archive containing the file built by
            [`modules_prepare`](#modules_prepare)""",
        "collect_unstripped_modules": "[`kernel_build.collect_unstripped_modules`](#kernel_build-collect_unstripped_modules)",
        "strip_modules": "[`kernel_build.strip_modules`](#kernel_build-strip_modules)",
        "src_protected_modules_list": """Source file with list of protected modules whose exports
            are being protected and needs to be updated by `--update_protected_exports`.

            May be `None`.""",
        "ddk_module_defconfig_fragments": """[depset](https://bazel.build/extending/depsets) of
            [`File`](https://bazel.build/rules/lib/File)s containing
            [`kernel_build.ddk_module_defconfig_fragments`](#kernel_build-ddk_module_defconfig_fragments).""",
        "kernel_uapi_headers": """[depset](https://bazel.build/extending/depsets) of
            [`File`](https://bazel.build/rules/lib/File)s containing
            archives of UAPI headers.""",
        "arch": "[`kernel_build.arch`](#kernel_build-arch)",
        "env_setup_script": """A [depset](https://bazel.build/extending/depsets) of
            [`File`](https://bazel.build/rules/lib/File)s to replay the `kernel_config` environment.

            See [`KernelConfigInfo`](#KernelConfigInfo).""",
        "config_out_dir": """The output directory of `kernel_config`.""",
        "outs": """[depset](https://bazel.build/extending/depsets) of `kernel_build`'s
            `outs`.""",
        "internal_outs": """[depset](https://bazel.build/extending/depsets) of `kernel_build`'s
            `internal_outs`.""",
        "ruledir": """`ruledir` from `kernel_build` that signifies the root for
            `outs`, `implcit_outs`, `internal_outs`.""",
        "module_env_archive": "Archive preparing an environment to build modules. May be `None`.",
        "has_base_kernel": "Whether the original `kernel_build()` has a not-None `base_kernel`.",
    },
)

GcovInfo = provider(
    doc = """A provider providing information about --gcov.""",
    fields = {
        "gcno_mapping": "`gcno_mapping.json`",
        "gcno_dir": """A [`File`](https://bazel.build/rules/lib/File) directory;
        With the generated gcno files.
        """,
    },
)

KernelUnstrippedModulesInfo = provider(
    doc = "A provider that provides unstripped modules",
    fields = {
        "directories": """A [depset](https://bazel.build/extending/depsets) of
[`File`](https://bazel.build/rules/lib/File)s, where
each item points to a directory containing unstripped modules.

Order matters; earlier elements in the traverse order has higher priority. Hence,
this depset must have `order` argument specified.

For [`kernel_build()`](kernel.md#kernel_build), this is a directory containing unstripped in-tree modules.
- This is `None` if and only if `collect_unstripped_modules = False`
- Never `None` if and only if `collect_unstripped_modules = True`
- An empty directory if and only if `collect_unstripped_modules = True` and `module_outs` is empty

For an external [`kernel_module()`](kernel.md#kernel_module), this is a directory containing unstripped external modules.
- This is `None` if and only if the `kernel_build` argument has `collect_unstripped_modules = False`
- Never `None` if and only if the `kernel_build` argument has `collect_unstripped_modules = True`
""",
    },
)

KernelModuleKernelBuildInfo = provider(
    doc = "Information about the `kernel_build` that an external module builds upon.",
    fields = {
        "label": "Label of the `kernel_build` target",
        "ext_module_info": "`KernelBuildExtModuleInfo`",
        "serialized_env_info": "`KernelSerializedEnvInfo`",
        "images_info": "`KernelImagesInfo`",
    },
)

KernelModuleInfo = provider(
    doc = "A provider that provides installed external modules.",
    fields = {
        "kernel_build_infos": """`KernelModuleKernelBuildInfo` containing info about
            the `kernel_build` attribute of this module""",

        # TODO(b/256688440): Avoid depset[directory_with_structure] to_list
        "modules_staging_dws_depset": """A [depset](https://bazel.build/extending/depsets) of
            `directory_with_structure` containing staging kernel modules.
            Contains the lib/modules/* suffix.""",
        "kernel_uapi_headers_dws_depset": """A [depset](https://bazel.build/extending/depsets) of
            `directory_with_structure` containing UAPI headers to use the module.""",
        "files": "A [depset](https://bazel.build/extending/depsets) of output `*.ko` files.",
        "packages": """For `kernel_module` / `ddk_module`s, a
            [depset](https://bazel.build/extending/depsets) containing package name of
            the target.

            For other rules that contains multiple `kernel_module`s, a [depset] containing package
            names of all external modules in an unspecified order.""",
        "label": "Label to the `kernel_module` target.",
        "modules_order": """A [depset](https://bazel.build/extending/depsets) of `modules.order`
            files from ddk_module's, kernel_module, etc.
            It uses [`postorder`](https://bazel.build/rules/lib/builtins/depset) ordering (dependencies
            first).""",
    },
)

KernelModuleSetupInfo = provider(
    doc = """Like `KernelEnvInfo` but the setup script is a fragment.

    The setup script requires some pre-setup environment before running it.
    """,
    fields = {
        "inputs": """A [depset](https://bazel.build/extending/depsets) of inputs associated with
            the target platform.""",
        "setup": "setup script fragment to initialize the environment",
    },
)

KernelModuleDepInfo = provider(
    doc = "Info that a `kernel_module` expects on a `kernel_module` dependency.",
    fields = {
        "label": "Label of the target where the infos are from.",
        "kernel_module_setup_info": "`KernelModuleSetupInfo`",
        "module_symvers_info": "`ModuleSymversInfo`",
        "kernel_module_info": "`KernelModuleInfo`",
    },
)

ModuleSymversInfo = provider(
    doc = "A provider that provides `Module.symvers` for `modpost`.",
    fields = {
        "restore_paths": """A [depset](https://bazel.build/extending/depsets) of
            paths relative to `COMMON_OUT_DIR` where the `Module.symvers` files will be
            restored to by `KernelModuleSetupInfo`.""",
    },
)

KernelImagesInfo = provider(
    doc = "A provider that represents the expectation of [`kernel_images`](kernel.md#kernel_images) to [`kernel_build`](kernel.md#kernel_build)",
    fields = {
        "base_kernel_label": "Label of the `base_kernel` target, if exists",
        "outs": "A list of File object corresponding to the `outs` attribute (excluding `module_outs`, `implicit_outs` and `internal_outs`)",
        "base_kernel_files": """A [depset](https://bazel.build/extending/depsets) containing
            [Default outputs](https://docs.bazel.build/versions/main/skylark/rules.html#default-outputs)
            of the rule specified by `base_kernel`""",
    },
)

DdkSubmoduleInfo = provider(
    doc = "A provider that describes information about a DDK submodule or module.",
    fields = {
        "outs": """A [depset](https://bazel.build/extending/depsets) containing a struct with
            these keys:

            - `out` is the name of an output file
            - `src` is a label containing the label of the target declaring the output
             file.""",
        "srcs": """A [depset](https://bazel.build/extending/depsets) of source files to build the
            submodule.""",
        "kernel_module_deps": """A [depset](https://bazel.build/extending/depsets) of
            `KernelModuleDepInfo` of dependent targets of this submodules that are
            kernel_module's.""",
    },
)

DdkConfigInfo = provider(
    doc = "A provider that describes information of a `_ddk_config` target to dependent `_ddk_config` targets.",
    fields = {
        "kconfig": """A [depset](https://bazel.build/extending/depsets) containing the Kconfig file
            of this and its dependencies. Uses `postorder` ordering (dependencies first).""",
        "defconfig": """A [depset](https://bazel.build/extending/depsets) containing the Kconfig
            file of this and its dependencies. Uses `postorder` ordering (dependencies first).""",
    },
)

ImagesInfo = provider(
    doc = "Provider from individual *_image rule to [`kernel_images`](kernel.md#kernel_images) rule",
    fields = {
        "files_dict": """A dictionary, where keys are keys in
            [OutputGroupInfo](https://bazel.build/rules/lib/providers/OutputGroupInfo)
            for `kernel_images`,
            and values are [depsets](https://bazel.build/extending/depsets).
        """,
    },
)

KernelConfigInfo = provider(
    doc = "For `kernel_config` to provide files to replay the environment",
    fields = {
        "env_setup_script": "script from `kernel_env`",
    },
)
