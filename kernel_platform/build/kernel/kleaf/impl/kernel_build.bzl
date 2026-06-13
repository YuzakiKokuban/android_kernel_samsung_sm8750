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
Defines a kernel build target.
"""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(
    "//build/kernel/kleaf/artifact_tests:kernel_test.bzl",
    "kernel_build_test",
    "kernel_module_test",
)
load(":abi/base_kernel_utils.bzl", "base_kernel_utils")
load(":abi/force_add_vmlinux_utils.bzl", "force_add_vmlinux_utils")
load(":abi/trim_nonlisted_kmi_utils.bzl", "trim_nonlisted_kmi_utils")
load(":btf.bzl", "btf")
load(":cache_dir.bzl", "cache_dir")
load(
    ":common_providers.bzl",
    "GcovInfo",
    "KernelBuildAbiInfo",
    "KernelBuildExtModuleInfo",
    "KernelBuildFilegroupDeclInfo",
    "KernelBuildInTreeModulesInfo",
    "KernelBuildInfo",
    "KernelBuildMixedTreeInfo",
    "KernelBuildOriginalEnvInfo",
    "KernelBuildUapiInfo",
    "KernelBuildUnameInfo",
    "KernelCmdsInfo",
    "KernelConfigInfo",
    "KernelEnvAttrInfo",
    "KernelEnvMakeGoalsInfo",
    "KernelImagesInfo",
    "KernelSerializedEnvInfo",
    "KernelToolchainInfo",
    "KernelUnstrippedModulesInfo",
)
load(":compile_commands_utils.bzl", "compile_commands_utils")
load(
    ":constants.bzl",
    "MODULES_STAGING_ARCHIVE",
    "MODULE_ENV_ARCHIVE_SUFFIX",
)
load(":debug.bzl", "debug")
load(":file.bzl", "file")
load(":file_selector.bzl", "file_selector")
load(":gcov_utils.bzl", "gcov_attrs", "get_grab_gcno_step")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":kernel_config.bzl", "kernel_config")
load(":kernel_config_settings.bzl", "kernel_config_settings")
load(":kernel_env.bzl", "kernel_env")
load(":kernel_headers.bzl", "kernel_headers")
load(":kernel_uapi_headers.bzl", "kernel_uapi_headers")
load(":kgdb.bzl", "kgdb")
load(":kmi_symbol_list.bzl", _kmi_symbol_list = "kmi_symbol_list")
load(":modules_prepare.bzl", "modules_prepare")
load(":raw_kmi_symbol_list.bzl", "raw_kmi_symbol_list")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

# Outputs of a kernel_build rule needed to build kernel_* that depends on it
_kernel_build_internal_outs = [
    "Module.symvers",
    "include/config/kernel.release",
]

_KERNEL_BUILD_OUT_ATTRS = ("outs", "module_outs", "implicit_outs", "module_implicit_outs", "internal_outs")
_KERNEL_BUILD_MODULE_OUT_ATTRS = ("module_outs", "module_implicit_outs")

_MODULES_PREPARE_ARCHIVE = "modules_prepare_outdir.tar.gz"

def kernel_build(
        name,
        build_config,
        outs,
        keep_module_symvers = None,
        srcs = None,
        module_outs = None,
        implicit_outs = None,
        module_implicit_outs = None,
        generate_vmlinux_btf = None,
        deps = None,
        arch = None,
        base_kernel = None,
        make_goals = None,
        kconfig_ext = None,
        dtstree = None,
        kmi_symbol_list = None,
        protected_exports_list = None,
        protected_modules_list = None,
        additional_kmi_symbol_lists = None,
        trim_nonlisted_kmi = None,
        kmi_symbol_list_strict_mode = None,
        collect_unstripped_modules = None,
        enable_interceptor = None,
        kbuild_symtypes = None,
        toolchain_version = None,
        strip_modules = None,
        module_signing_key = None,
        system_trusted_key = None,
        modules_prepare_force_generate_headers = None,
        defconfig_fragments = None,
        page_size = None,
        pack_module_env = None,
        sanitizers = None,
        ddk_module_defconfig_fragments = None,
        **kwargs):
    """Defines a kernel build target with all dependent targets.

    It uses a `build_config` to construct a deterministic build environment (e.g.
    `common/build.config.gki.aarch64`). The kernel sources need to be declared
    via srcs (using a `glob()`). outs declares the output files that are surviving
    the build. The effective output file names will be
    `$(name)/$(output_file)`. Any other artifact is not guaranteed to be
    accessible after the rule has run. The default `toolchain_version` is defined
    with the value in `common/build.config.constants`, but can be overriden.

    A few additional labels are generated.
    For example, if name is `"kernel_aarch64"`:
    - `kernel_aarch64_uapi_headers` provides the UAPI kernel headers.
    - `kernel_aarch64_headers` provides the kernel headers.

    Args:
        name: The final kernel target name, e.g. `"kernel_aarch64"`.
        build_config: Label of the build.config file, e.g. `"build.config.gki.aarch64"`.
        kconfig_ext: Label of an external Kconfig.ext file sourced by the GKI kernel.
        keep_module_symvers: If set to True, a copy of the default output `Module.symvers` is kept.
          * To avoid collisions in mixed build distribution packages, the file is renamed
            as `$(name)_Module.symvers`.
          * Default is False.
        srcs: The kernel sources (a `glob()`). If unspecified or `None`, it is the following:
          ```
          glob(
              ["**"],
              exclude = [
                  "**/.*",          # Hidden files
                  "**/.*/**",       # Files in hidden directories
                  "**/BUILD.bazel", # build files
                  "**/*.bzl",       # build files
              ],
          )
          ```
        arch: [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes).
          Target architecture. Default is `arm64`.

          Value should be one of:
          * `arm64`
          * `x86_64`
          * `riscv64`
          * `arm` (for 32-bit, uncommon)
          * `i386` (for 32-bit, uncommon)

          This must be consistent to `ARCH` in build configs if the latter
          is specified. Otherwise, a warning / error may be raised.

        base_kernel: A label referring the base kernel build.

          If set, the list of files specified in the `DefaultInfo` of the rule specified in
          `base_kernel` is copied to a directory, and `KBUILD_MIXED_TREE` is set to the directory.
          Setting `KBUILD_MIXED_TREE` effectively enables mixed build.

          To set additional flags for mixed build, change `build_config` to a `kernel_build_config`
          rule, with a build config fragment that contains the additional flags.

          The label specified by `base_kernel` must produce a list of files similar
          to what a `kernel_build` rule does. Usually, this points to one of the following:
          - `//common:kernel_{arch}`
          - A `kernel_filegroup` rule, e.g.
            ```
            load("//build/kernel/kleaf:constants.bzl, "DEFAULT_GKI_OUTS")
            kernel_filegroup(
              name = "my_kernel_filegroup",
              srcs = DEFAULT_GKI_OUTS,
            )
            ```
        make_goals: A list of strings defining targets for the kernel build.
          This overrides `MAKE_GOALS` from build config if provided.
        generate_vmlinux_btf: If `True`, generates `vmlinux.btf` that is stripped of any debug
          symbols, but contains type and symbol information within a .BTF section.
          This is suitable for ABI analysis through BTF.

          Requires that `"vmlinux"` is in `outs`.
        deps: Additional dependencies to build this kernel.
        module_outs: A list of in-tree drivers. Similar to `outs`, but for `*.ko` files.

          If a `*.ko` kernel module should not be copied to `${DIST_DIR}`, it must be
          included `implicit_outs` instead of `module_outs`. The list `implicit_outs + module_outs`
          must include **all** `*.ko` files in `${OUT_DIR}`. If not, a build error is raised.

          Like `outs`, `module_outs` are part of the
          [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html)
          that this `kernel_build` returns. For example:
          ```
          kernel_build(name = "kernel", module_outs = ["foo.ko"], ...)
          copy_to_dist_dir(name = "kernel_dist", data = [":kernel"])
          ```
          `foo.ko` will be included in the distribution.

          Like `outs`, this may be a `dict`. If so, it is wrapped in
          [`select()`](https://docs.bazel.build/versions/main/configurable-attributes.html). See
          documentation for `outs` for more details.
        outs: The expected output files.

          Note: in-tree modules should be specified in `module_outs` instead.

          This attribute must be either a `dict` or a `list`. If it is a `list`, for each item
          in `out`:

          - If `out` does not contain a slash, the build rule
            automatically finds a file with name `out` in the kernel
            build output directory `${OUT_DIR}`.
            ```
            find ${OUT_DIR} -name {out}
            ```
            There must be exactly one match.
            The file is copied to the following in the output directory
            `{name}/{out}`

            Example:
            ```
            kernel_build(name = "kernel_aarch64", outs = ["vmlinux"])
            ```
            The bulid system copies `${OUT_DIR}/[<optional subdirectory>/]vmlinux`
            to `kernel_aarch64/vmlinux`.
            `kernel_aarch64/vmlinux` is the label to the file.

          - If `out` contains a slash, the build rule locates the file in the
            kernel build output directory `${OUT_DIR}` with path `out`
            The file is copied to the following in the output directory
              1. `{name}/{out}`
              2. `{name}/$(basename {out})`

            Example:
            ```
            kernel_build(
              name = "kernel_aarch64",
              outs = ["arch/arm64/boot/vmlinux"])
            ```
            The bulid system copies
              `${OUT_DIR}/arch/arm64/boot/vmlinux`
            to:
              - `kernel_aarch64/arch/arm64/boot/vmlinux`
              - `kernel_aarch64/vmlinux`
            They are also the labels to the output files, respectively.

            See `search_and_cp_output.py` for details.

          Files in `outs` are part of the
          [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html)
          that this `kernel_build` returns. For example:
          ```
          kernel_build(name = "kernel", outs = ["vmlinux"], ...)
          copy_to_dist_dir(name = "kernel_dist", data = [":kernel"])
          ```
          `vmlinux` will be included in the distribution.

          If it is a `dict`, it is wrapped in
          [`select()`](https://docs.bazel.build/versions/main/configurable-attributes.html).

          Example:
          ```
          kernel_build(
            name = "kernel_aarch64",
            outs = {"config_foo": ["vmlinux"]})
          ```
          If conditions in `config_foo` is met, the rule is equivalent to
          ```
          kernel_build(
            name = "kernel_aarch64",
            outs = ["vmlinux"])
          ```
          As explained above, the bulid system copies `${OUT_DIR}/[<optional subdirectory>/]vmlinux`
          to `kernel_aarch64/vmlinux`.
          `kernel_aarch64/vmlinux` is the label to the file.

          Note that a `select()` may not be passed into `kernel_build()` because
          [`select()` cannot be evaluated in macros](https://docs.bazel.build/versions/main/configurable-attributes.html#why-doesnt-select-work-in-macros).
          Hence:
          - [combining `select()`s](https://docs.bazel.build/versions/main/configurable-attributes.html#combining-selects)
            is not allowed. Instead, expand the cartesian product.
          - To use
            [`AND` chaining](https://docs.bazel.build/versions/main/configurable-attributes.html#or-chaining)
            or
            [`OR` chaining](https://docs.bazel.build/versions/main/configurable-attributes.html#selectsconfig_setting_group),
            use `selects.config_setting_group()`.

        implicit_outs: Like `outs`, but not copied to the distribution directory.

          Labels are created for each item in `implicit_outs` as in `outs`.

        module_implicit_outs: like `module_outs`, but not copied to the distribution directory.

          Labels are created for each item in `module_implicit_outs` as in `outs`.

        kmi_symbol_list: A label referring to the main KMI symbol list file. See `additional_kmi_symbol_lists`.

          This is the Bazel equivalent of `ADDITIONAL_KMI_SYMBOL_LISTS`.
        additional_kmi_symbol_lists: A list of labels referring to additional KMI symbol list files.

          This is the Bazel equivalent of `ADDITIONAL_KMI_SYMBOL_LISTS`.

          Let
          ```
          all_kmi_symbol_lists = [kmi_symbol_list] + additional_kmi_symbol_list
          ```

          If `all_kmi_symbol_lists` is a non-empty list, `abi_symbollist` and
          `abi_symbollist.report` are created and added to the
          [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html),
          and copied to `DIST_DIR` during distribution.

          If `all_kmi_symbol_lists` is `None` or an empty list, `abi_symbollist` and
          `abi_symbollist.report` are not created.

          It is possible to use a `glob()` to determine whether `abi_symbollist`
          and `abi_symbollist.report` should be generated at build time.
          For example:
          ```
          kmi_symbol_list = "android/abi_gki_aarch64",
          additional_kmi_symbol_lists = glob(["android/abi_gki_aarch64*"], exclude = ["android/abi_gki_aarch64"]),
          ```

        protected_exports_list: A file containing list of protected exports.
          For example:
          ```
          protected_exports_list = "//common:android/abi_gki_protected_exports"
          ```

        protected_modules_list: A file containing list of protected modules,
          For example:
          ```
          protected_modules_list = "//common:android/gki_protected_modules"
          ```

        trim_nonlisted_kmi: If `True`, trim symbols not listed in
          `kmi_symbol_list` and `additional_kmi_symbol_lists`.
          This is the Bazel equivalent of `TRIM_NONLISTED_KMI`.

          Requires `all_kmi_symbol_lists` to be non-empty. If `kmi_symbol_list`
          or `additional_kmi_symbol_lists`
          is a `glob()`, it is possible to set `trim_nonlisted_kmi` to be a
          value based on that `glob()`. For example:
          ```
          trim_nonlisted_kmi = len(glob(["android/abi_gki_aarch64*"])) > 0
          ```
        kmi_symbol_list_strict_mode: If `True`, add a build-time check between
          `[kmi_symbol_list] + additional_kmi_symbol_lists`
          and the KMI resulting from the build, to ensure
          they match 1-1.
        collect_unstripped_modules: If `True`, provide all unstripped in-tree.
        enable_interceptor: If set to `True`, enable interceptor so it can be
          used in [`kernel_compile_commands`](#kernel_compile_commands).
        kbuild_symtypes: The value of `KBUILD_SYMTYPES`.

          This can be set to one of the following:

          - `"true"`
          - `"false"`
          - `"auto"`
          - `None`, which defaults to `"auto"`

          If the value is `"auto"`, it is determined by the `--kbuild_symtypes`
          flag.

          If the value is `"true"`; or the value is `"auto"` and
          `--kbuild_symtypes` is specified, then `KBUILD_SYMTYPES=1`.
          **Note**: kernel build time can be significantly longer.

          If the value is `"false"`; or the value is `"auto"` and
          `--kbuild_symtypes` is not specified, then `KBUILD_SYMTYPES=`.
        toolchain_version: [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes).
          The toolchain version to depend on.
        strip_modules: If `None` or not specified, default is `False`.
          If set to `True`, debug information for distributed modules is stripped.

          This corresponds to negated value of `DO_NOT_STRIP_MODULES` in `build.config`.
        module_signing_key: A label referring to a module signing key.

          This is to allow for dynamic setting of `CONFIG_MODULE_SIG_KEY` from Bazel.
        system_trusted_key: A label referring to a trusted system key.

          This is to allow for dynamic setting of `CONFIG_SYSTEM_TRUSTED_KEY` from Bazel.
        dtstree: Device tree support.
        modules_prepare_force_generate_headers: If `True` it forces generation of
          additional headers as part of modules_prepare.
        defconfig_fragments: A list of targets that are applied to the defconfig.

          As a convention, files should usually be named `<prop>_defconfig`
          (e.g. `kasan_defconfig`) or `<prop>_<value>_defconfig` (e.g. `lto_none_defconfig`)
          to provide human-readable hints during the build. The prefix should
          describe what the defconfig does. However, this is not a requirement.
          These configs are also applied to external modules, including
          `kernel_module`s and `ddk_module`s.

          **NOTE**: `defconfig_fragments` are applied **after** `make defconfig`, similar
          to `POST_DEFCONFIG_CMDS`. If you migrate from `PRE_DEFCONFIG_CMDS`
          to `defconfig_fragments`, certain values may change; double check
          by building the `<target_name>_config` target and examining the
          generated `.config` file.
        page_size: Default is `"default"`. Page size of the kernel build.

          Value may be one of `"default"`, `"4k"`, `"16k"` or `"64k"`. If
          `"default"`, the defconfig is left as-is.

          16k / 64k page size is only supported on `arch = "arm64"`.
        pack_module_env: If `True`, create `{name}_module_env.tar.gz`
          and other archives as part of the default output of this target.

          These archives contains necessary files to build external modules.
        sanitizers: **non-configurable**. A list of sanitizer configurations.
          By default, no sanitizers are explicity configured; values in defconfig are
          respected. Possible values are:
            - `["kasan_any_mode"]`
            - `["kasan_sw_tags"]`
            - `["kasan_generic"]`
            - `["kcsan"]`
        ddk_module_defconfig_fragments: A list of additional defconfigs, to be used
          in `ddk_module`s building against this kernel.
          Unlike `defconfig_fragments`, `ddk_module_defconfig_fragments` is not applied
          to this `kernel_build` target, nor dependent legacy `kernel_module`s.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    env_target_name = name + "_env"
    config_target_name = name + "_config"
    modules_prepare_target_name = name + "_modules_prepare"
    uapi_headers_target_name = name + "_uapi_headers"
    headers_target_name = name + "_headers"
    src_kmi_symbol_list_target_name = name + "_src_kmi_symbol_list"
    kmi_symbol_list_target_name = name + "_kmi_symbol_list"
    abi_symbollist_target_name = name + "_kmi_symbol_list_abi_symbollist"
    raw_kmi_symbol_list_target_name = name + "_raw_kmi_symbol_list"

    # Currently only support one sanitizer
    if sanitizers and len(sanitizers) > 1:
        fail("only one sanitizer may be passed to kernel_build.sanitizers")

    if srcs == None:
        srcs = native.glob(
            ["**"],
            exclude = [
                "**/.*",
                "**/.*/**",
                "**/BUILD.bazel",
                "**/*.bzl",
            ],
        )

    if strip_modules == None:
        strip_modules = False

    if arch == None:
        arch = "arm64"

    trim_nonlisted_kmi_str = str(trim_nonlisted_kmi)
    trim_nonlisted_kmi = trim_nonlisted_kmi_utils.selected_attr(trim_nonlisted_kmi)

    internal_kwargs = dict(kwargs)
    internal_kwargs.pop("visibility", None)

    kwargs_with_manual = dict(kwargs)
    kwargs_with_manual["tags"] = ["manual"]

    lto = select({
        Label("//build/kernel/kleaf:lto_is_none"): "none",
        Label("//build/kernel/kleaf:lto_is_thin"): "thin",
        Label("//build/kernel/kleaf:lto_is_full"): "full",
        Label("//build/kernel/kleaf:lto_is_fast"): "fast",
        # TODO(b/229662633): Allow kernel_build() macro to set this value.
        Label("//build/kernel/kleaf:lto_is_default"): "default",
    })

    defconfig_fragments = _get_defconfig_fragments(
        kernel_build_name = name,
        kernel_build_defconfig_fragments = defconfig_fragments,
        kernel_build_arch = arch,
        kernel_build_page_size = page_size,
        kernel_build_sanitizers = sanitizers,
        kernel_build_trim_nonlisted_kmi = trim_nonlisted_kmi_str,
        **internal_kwargs
    )
 
    toolchain_constraints = []
    if toolchain_version != None:
        toolchain_constraint = Label("//prebuilts/clang/host/linux-x86/kleaf:{}".format(toolchain_version))
        toolchain_constraints.append(Label(toolchain_constraint))
    else:
        # use default toolchain, e.g.
        # //prebuilts/clang/host/linux-x86/kleaf:android_arm64_clang_toolchain
        pass

    native.platform(
        name = name + "_platform_target",
        constraint_values = [
            Label("@platforms//os:android"),
            Label("@platforms//cpu:{}".format(arch)),
        ] + toolchain_constraints,
        **internal_kwargs
    )

    native.platform(
        name = name + "_platform_exec",
        constraint_values = [
            Label("@platforms//os:linux"),
            Label("@platforms//cpu:x86_64"),
        ] + toolchain_constraints,
        **internal_kwargs
    )

    kernel_env(
        name = env_target_name,
        build_config = build_config,
        kconfig_ext = kconfig_ext,
        dtstree = dtstree,
        srcs = srcs,
        kbuild_symtypes = kbuild_symtypes,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        lto = lto,
        make_goals = make_goals,
        target_platform = name + "_platform_target",
        exec_platform = name + "_platform_exec",
        defconfig_fragments = defconfig_fragments,
        **internal_kwargs
    )

    # Wrap in a target so kmi_symbol_list is configurable. A select() value cannot be
    # embedded in the all_kmi_symbol_lists below.
    file(
        name = src_kmi_symbol_list_target_name,
        src = kmi_symbol_list,
        **internal_kwargs
    )

    all_kmi_symbol_lists = [src_kmi_symbol_list_target_name]
    if additional_kmi_symbol_lists:
        all_kmi_symbol_lists += additional_kmi_symbol_lists

    _kmi_symbol_list(
        name = kmi_symbol_list_target_name,
        env = env_target_name,
        srcs = all_kmi_symbol_lists,
        **internal_kwargs
    )

    native.filegroup(
        name = abi_symbollist_target_name,
        srcs = [kmi_symbol_list_target_name],
        output_group = "abi_symbollist",
        **internal_kwargs
    )

    raw_kmi_symbol_list(
        name = raw_kmi_symbol_list_target_name,
        env = env_target_name,
        src = abi_symbollist_target_name,
        **internal_kwargs
    )

    kernel_config(
        name = config_target_name,
        env = env_target_name,
        srcs = srcs,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        raw_kmi_symbol_list = raw_kmi_symbol_list_target_name,
        module_signing_key = module_signing_key,
        system_trusted_key = system_trusted_key,
        lto = lto,
        defconfig_fragments = defconfig_fragments,
        **internal_kwargs
    )

    modules_prepare(
        name = modules_prepare_target_name,
        config = config_target_name,
        srcs = srcs,
        outdir_tar_gz = modules_prepare_target_name + "/" + _MODULES_PREPARE_ARCHIVE,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        force_generate_headers = modules_prepare_force_generate_headers,
        **internal_kwargs
    )

    _kernel_build(
        name = name,
        config = config_target_name,
        keep_module_symvers = keep_module_symvers,
        srcs = srcs,
        outs = kernel_utils.transform_kernel_build_outs(name, "outs", outs),
        module_outs = kernel_utils.transform_kernel_build_outs(name, "module_outs", module_outs),
        implicit_outs = kernel_utils.transform_kernel_build_outs(name, "implicit_outs", implicit_outs),
        module_implicit_outs = kernel_utils.transform_kernel_build_outs(name, "module_implicit_outs", module_implicit_outs),
        internal_outs = kernel_utils.transform_kernel_build_outs(name, "internal_outs", _kernel_build_internal_outs),
        deps = deps,
        base_kernel = base_kernel,
        modules_prepare = modules_prepare_target_name,
        kmi_symbol_list_strict_mode = kmi_symbol_list_strict_mode,
        raw_kmi_symbol_list = raw_kmi_symbol_list_target_name,
        kernel_uapi_headers = uapi_headers_target_name,
        collect_unstripped_modules = collect_unstripped_modules,
        combined_abi_symbollist = abi_symbollist_target_name,
        enable_interceptor = enable_interceptor,
        strip_modules = strip_modules,
        src_protected_exports_list = protected_exports_list,
        src_protected_modules_list = protected_modules_list,
        src_kmi_symbol_list = kmi_symbol_list,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        pack_module_env = pack_module_env,
        sanitizers = sanitizers,
        ddk_module_defconfig_fragments = ddk_module_defconfig_fragments,
        arch = arch,
        **kwargs
    )

    # key = attribute name, value = a list of labels for that attribute
    real_outs = {}

    for out_name, out_attr_val in (
        ("outs", outs),
        ("module_outs", module_outs),
        ("implicit_outs", implicit_outs),
        ("module_implicit_outs", module_implicit_outs),
        # internal_outs are opaque to the user, hence we don't create a alias (filegroup) for them.
    ):
        if out_attr_val == None:
            continue
        if type(out_attr_val) == type([]):
            for out in out_attr_val:
                native.filegroup(name = name + "/" + out, srcs = [":" + name], output_group = out, **kwargs)
                if out != paths.basename(out):
                    native.filegroup(name = name + "/" + paths.basename(out), srcs = [":" + name], output_group = out, **kwargs)
            real_outs[out_name] = [name + "/" + out for out in out_attr_val]
        elif type(out_attr_val) == type({}):
            # out_attr_val = {config_setting: [out, ...], ...}
            # => reverse_dict = {out: [config_setting, ...], ...}
            for out, config_settings in utils.reverse_dict(out_attr_val).items():
                native.filegroup(
                    name = name + "/" + out,
                    # Use a select() to prevent this rule to build when config_setting is not fulfilled.
                    srcs = select({
                        config_setting: [":" + name]
                        for config_setting in config_settings
                    }),
                    output_group = out,
                    # Use "manual" tags to prevent it to be built with ...
                    **kwargs_with_manual
                )
                if out != paths.basename(out):
                    native.filegroup(
                        name = name + "/" + paths.basename(out),
                        # Use a select() to prevent this rule to build when config_setting is not fulfilled.
                        srcs = select({
                            config_setting: [":" + name]
                            for config_setting in config_settings
                        }),
                        output_group = out,
                        # Use "manual" tags to prevent it to be built with ...
                        **kwargs_with_manual
                    )
            real_outs[out_name] = [name + "/" + out for out, _ in utils.reverse_dict(out_attr_val).items()]
        else:
            fail("Unexpected type {} for {}: {}".format(type(out_attr_val), out_name, out_attr_val))

    kernel_uapi_headers(
        name = uapi_headers_target_name,
        config = config_target_name,
        srcs = srcs,
        **kwargs
    )

    kernel_headers(
        name = headers_target_name,
        kernel_build = name,
        env = env_target_name,
        # TODO: We need arch/ and include/ only.
        srcs = srcs,
        **kwargs
    )

    if generate_vmlinux_btf:
        btf_name = name + "_btf"
        btf(
            name = btf_name,
            vmlinux = name + "/vmlinux",
            env = env_target_name,
            **kwargs
        )

    kernel_build_test(
        name = name + "_test",
        target = name,
        **kwargs
    )

    kernel_module_test(
        name = name + "_modules_test",
        modules = (real_outs.get("module_outs") or []) + (real_outs.get("module_implicit_outs") or []),
        **kwargs
    )

# buildifier: disable=print
def _skip_build_checks(ctx, what):
    # Skip for these flags as they are usually debug targets.
    for flag in (
        "kasan",
        "kasan_sw_tags",
        "kasan_generic",
        "kcsan",
        "kgdb",
        "debug",
        "gcov",
    ):
        if getattr(ctx.attr, "_" + flag)[BuildSettingInfo].value:
            print("\nWARNING: {this_label}: {what} was \
IGNORED because --{flag} is set!".format(this_label = ctx.label, what = what, flag = flag))
            return True

    if ctx.attr.sanitizers[0] != "default":
        print("\nWARNING: {this_label}: {what} was \
IGNORED because kernel_build.sanitizers is set!".format(this_label = ctx.label, what = what))
        return True

    return False

def _get_defconfig_fragments(
        kernel_build_name,
        kernel_build_defconfig_fragments,
        kernel_build_arch,
        kernel_build_page_size,
        kernel_build_sanitizers,
        kernel_build_trim_nonlisted_kmi,
        **internal_kwargs):
    # Use a separate list to avoid .append on the provided object directly.
    # kernel_build_defconfig_fragments could be a list or a select() expression.
    additional_fragments = [
        Label("//build/kernel/kleaf:defconfig_fragment"),
        Label("//build/kernel/kleaf/impl/defconfig:debug"),
        Label("//build/kernel/kleaf/impl/defconfig:gcov"),
        Label("//build/kernel/kleaf/impl/defconfig:rust"),
        Label("//build/kernel/kleaf/impl/defconfig:zstd_dwarf_compression"),
    ]

    btf_debug_info_target = kernel_build_name + "_defconfig_fragment_btf_debug_info"
    file_selector(
        name = btf_debug_info_target,
        first_selector = select({
            Label("//build/kernel/kleaf:btf_debug_info_is_enabled"): "enable",
            Label("//build/kernel/kleaf:btf_debug_info_is_disabled"): "disable",
            # TODO(b/229662633): Add kernel_build.btf_debug_info. After that, this should be
            #   `kernel_build_btf_debug_info or "enable"`.
            "//conditions:default": "default",
        }),
        files = {
            Label("//build/kernel/kleaf/impl/defconfig:btf_debug_info_enabled_defconfig"): "enable",
            Label("//build/kernel/kleaf/impl/defconfig:btf_debug_info_disabled_defconfig"): "disable",
            # If --btf_debug_info=default, do not apply any defconfig fragments
            Label("//build/kernel/kleaf/impl:empty_filegroup"): "default",
        },
        **internal_kwargs
    )
    additional_fragments.append(btf_debug_info_target)

    page_size_target = kernel_build_name + "_defconfig_fragment_page_size"
    file_selector(
        name = page_size_target,
        first_selector = select({
            Label("//build/kernel/kleaf:page_size_4k"): "4k",
            Label("//build/kernel/kleaf:page_size_16k"): "16k",
            Label("//build/kernel/kleaf:page_size_64k"): "64k",
            # If --page_size=default, use kernel_build.page_size; If kernel_build.page_size
            # is also unset, use "default".
            "//conditions:default": None,
        }),
        second_selector = kernel_build_page_size,
        third_selector = "default",
        files = {
            Label("//build/kernel/kleaf/impl/defconfig:{}_4k_defconfig".format(kernel_build_arch)): "4k",
            Label("//build/kernel/kleaf/impl/defconfig:{}_16k_defconfig".format(kernel_build_arch)): "16k",
            Label("//build/kernel/kleaf/impl/defconfig:{}_64k_defconfig".format(kernel_build_arch)): "64k",
            # If --page_size=default, do not apply any defconfig fragments
            Label("//build/kernel/kleaf/impl:empty_filegroup"): "default",
        },
        **internal_kwargs
    )
    additional_fragments.append(page_size_target)

    module_protection_target = kernel_build_name + "_defconfig_fragment_module_protection"

    # When the value is not specified in the kernel_build rule, do nothing.
    if kernel_build_trim_nonlisted_kmi == "None":
        kernel_build_trim_nonlisted_kmi = "True"
    file_selector(
        name = module_protection_target,
        first_selector = select({
            Label("//build/kernel/kleaf/impl:force_disable_trim_is_true"): "False",
            Label("//build/kernel/kleaf:debug_is_true"): "False",
            Label("//build/kernel/kleaf:gcov_is_true"): "False",
            Label("//build/kernel/kleaf:kasan_is_true"): "False",
            Label("//build/kernel/kleaf:kcsan_is_true"): "False",
            Label("//build/kernel/kleaf:kgdb_is_true"): "False",
            "//conditions:default": None,
        }),
        second_selector = kernel_build_trim_nonlisted_kmi,
        files = {
            Label("//build/kernel/kleaf/impl/defconfig:gki_module_protection_disabled_defconfig"): "False",
            Label("//build/kernel/kleaf/impl:empty_filegroup"): "True",
        },
        **internal_kwargs
    )
    additional_fragments.append(module_protection_target)

    kernel_build_sanitizer = "default"
    if kernel_build_sanitizers:
        kernel_build_sanitizer = kernel_build_sanitizers[0]

    sanitizer_target = kernel_build_name + "_defconfig_fragment_sanitizer"
    file_selector(
        name = sanitizer_target,
        first_selector = select({
            Label("//build/kernel/kleaf/impl:kasan_any_mode_is_set_to_true"): "kasan_any_mode",
            Label("//build/kernel/kleaf/impl:kasan_sw_tags_is_set_to_true"): "kasan_sw_tags",
            Label("//build/kernel/kleaf/impl:kasan_generic_is_set_to_true"): "kasan_generic",
            Label("//build/kernel/kleaf/impl:kcsan_is_set_to_true"): "kcsan",
            "//conditions:default": None,
        }),
        second_selector = kernel_build_sanitizer,
        third_selector = "default",
        files = {
            Label("//build/kernel/kleaf/impl/defconfig:kasan_any_mode"): "kasan_any_mode",
            Label("//build/kernel/kleaf/impl/defconfig:{}_kasan_sw_tags".format(kernel_build_arch)): "kasan_sw_tags",
            Label("//build/kernel/kleaf/impl/defconfig:kasan_generic"): "kasan_generic",
            Label("//build/kernel/kleaf/impl/defconfig:kcsan"): "kcsan",
            Label("//build/kernel/kleaf/impl:empty_filegroup"): "default",
        },
        **internal_kwargs
    )
    additional_fragments.append(sanitizer_target)

    if kernel_build_defconfig_fragments == None:
        kernel_build_defconfig_fragments = []

    # Do not call kernel_build_defconfig_fragments += ... to avoid
    # modifying the incoming object from kernel_build.defconfig_fragments.
    defconfig_fragments = kernel_build_defconfig_fragments + additional_fragments
    return defconfig_fragments

def _uniq(lst):
    """Deduplicates items in lst."""
    return sets.to_list(sets.make(lst))

def _progress_message_suffix(ctx):
    """Returns suffix for all progress messages for kernel_build."""
    return "{}{}".format(
        ctx.attr.config[KernelEnvAttrInfo].progress_message_note,
        ctx.label,
    )

def _create_kbuild_mixed_tree(ctx):
    """Adds actions that creates the `KBUILD_MIXED_TREE`."""
    hermetic_tools = hermetic_toolchain.get(ctx)
    base_kernel_files = depset()
    outputs = []
    kbuild_mixed_tree = None
    cmd = ""
    arg = ""
    if base_kernel_utils.get_base_kernel(ctx):
        # Create a directory for KBUILD_MIXED_TREE. Flatten the directory structure of the files
        # that base_kernel_utils.get_base_kernel(ctx) provides. declare_directory is sufficient because the directory should
        # only change when the dependent base_kernel_utils.get_base_kernel(ctx) changes.
        kbuild_mixed_tree = ctx.actions.declare_directory("{}_kbuild_mixed_tree".format(ctx.label.name))
        outputs = [kbuild_mixed_tree]
        base_kernel_files = base_kernel_utils.get_base_kernel(ctx)[KernelBuildMixedTreeInfo].files
        kbuild_mixed_tree_command = hermetic_tools.setup + """
          # Restore GKI artifacts for mixed build
            export KBUILD_MIXED_TREE=$(realpath {kbuild_mixed_tree})
            rm -rf ${{KBUILD_MIXED_TREE}}
            mkdir -p ${{KBUILD_MIXED_TREE}}
            for base_kernel_file in {base_kernel_files}; do
              cp -a -t ${{KBUILD_MIXED_TREE}} $(readlink -m ${{base_kernel_file}})
            done
        """.format(
            base_kernel_files = " ".join([file.path for file in base_kernel_files.to_list()]),
            kbuild_mixed_tree = kbuild_mixed_tree.path,
        )
        debug.print_scripts(ctx, kbuild_mixed_tree_command, what = "kbuild_mixed_tree")
        ctx.actions.run_shell(
            mnemonic = "KernelBuildKbuildMixedTree",
            inputs = depset(transitive = [base_kernel_files]),
            outputs = [kbuild_mixed_tree],
            tools = hermetic_tools.deps,
            progress_message = "Creating KBUILD_MIXED_TREE {}".format(_progress_message_suffix(ctx)),
            command = kbuild_mixed_tree_command,
        )

        cmd = """
            export KBUILD_MIXED_TREE=$(realpath {kbuild_mixed_tree})
        """.format(
            kbuild_mixed_tree = kbuild_mixed_tree.path,
        )

        arg = "--srcdir ${KBUILD_MIXED_TREE}"
    return struct(
        outputs = outputs,
        cmd = cmd,
        base_kernel_files = base_kernel_files,
        arg = arg,
    )

def _get_base_kernel_all_module_names(ctx):
    """Returns the file containing all module names from the base kernel or `[]` if there's no base_kernel."""
    base_kernel_for_module_names = base_kernel_utils.get_base_kernel_for_module_names(ctx)
    if base_kernel_for_module_names:
        return base_kernel_for_module_names[KernelBuildInTreeModulesInfo].all_module_names
    return []

def _get_out_attr_vals(ctx):
    """Common implementation for getting all ctx.attr.*out.

    This function should be used instead of actually inspecting ctx.attr.*out, because
    this function also handles cases with additional outputs added by config settings.
    """
    attr_vals = {attr: getattr(ctx.attr, attr) for attr in _KERNEL_BUILD_OUT_ATTRS}

    # The list is immutable, so use x = x + ... to make a copy
    attr_vals["outs"] = attr_vals["outs"] + force_add_vmlinux_utils.additional_outs(ctx)

    return attr_vals

def _declare_all_output_files(ctx):
    """Declares output files based on `ctx.attr.*outs`."""
    attr_vals = _get_out_attr_vals(ctx)

    # kernel_build(name="kernel", outs=["out"])
    # => _kernel_build(name="kernel", outs=["kernel/out"], internal_outs=["kernel/Module.symvers", ...])
    # => all_output_names = ["foo", "Module.symvers", ...]
    #    all_output_files = {"out": {"foo": File(...)}, "internal_outs": {"Module.symvers": File(...)}, ...}
    all_output_files = {}
    for attr, val in attr_vals.items():
        all_output_files[attr] = {
            name: ctx.actions.declare_file("{}/{}".format(ctx.label.name, name))
            for name in val
        }

    return all_output_files

def _split_out_attrs(ctx):
    """Partitions items in *outs into two lists: non-modules and modules."""
    non_modules = []
    modules = []
    attr_vals = _get_out_attr_vals(ctx)
    for attr, val in attr_vals.items():
        if attr in _KERNEL_BUILD_MODULE_OUT_ATTRS:
            modules += val
        else:
            non_modules += val
    return struct(
        non_modules = non_modules,
        modules = modules,
    )

def _write_module_names_to_file(ctx, filename, names):
    """Adds an action that writes |names| to a file named |filename|. Each item occupies a line."""
    all_module_names_file = ctx.actions.declare_file("{}_all_module_names/{}".format(ctx.label.name, filename))
    ctx.actions.write(
        output = all_module_names_file,
        content = "\n".join(names) + "\n",
    )
    return all_module_names_file

# A "step" contains these fields:
#
# * inputs: a list of source files for this step
# * tools: a list of required tools for this step
# * outputs: a list of generated files for this step
# * cmd (optional): the command for this step
# * Other special fields.
#
# In other words, a step is a weaker form of an [Action](https://bazel.build/rules/lib/Action),
# but because the `OUT_DIR` needs to be kept between the steps, they are stuffed into the main
# action.

def _get_interceptor_step(ctx):
    """Returns a step for interceptor.

    This is a special step that doesn't have a `cmd`, but provides a `command_prefix` instead.

    Returns:
      A struct with these fields:

      * inputs
      * tools
      * outputs
      * command_prefix
      * output_file
    """
    interceptor_output = None
    interceptor_command_prefix = ""
    if ctx.attr.enable_interceptor:
        interceptor_output = ctx.actions.declare_file("{name}/interceptor_output.bin".format(name = ctx.label.name))
        interceptor_command_prefix = "interceptor -r -l {interceptor_output} --".format(
            interceptor_output = interceptor_output.path,
        )
    return struct(
        inputs = [],
        tools = [],
        outputs = [interceptor_output] if interceptor_output else [],
        command_prefix = interceptor_command_prefix,
        output_file = interceptor_output,
    )

def _get_grab_intree_modules_step(ctx, has_any_modules, modules_staging_dir, ruledir, all_module_names):
    """Returns a step for grabbing the in-tree modules from `OUT_DIR`.

    Returns:
      A struct with these fields:

      * inputs
      * tools
      * cmd
      * outputs
    """
    tools = []
    grab_intree_modules_cmd = ""
    if has_any_modules:
        tools.append(ctx.executable._search_and_cp_output)
        grab_intree_modules_cmd = """
            {search_and_cp_output} --srcdir {modules_staging_dir}/lib/modules/*/kernel --dstdir {ruledir} {all_module_names}
        """.format(
            search_and_cp_output = ctx.executable._search_and_cp_output.path,
            modules_staging_dir = modules_staging_dir,
            ruledir = ruledir,
            all_module_names = " ".join(all_module_names),
        )
    return struct(
        inputs = [],
        tools = tools,
        cmd = grab_intree_modules_cmd,
        outputs = [],
    )

def _get_grab_unstripped_modules_step(ctx, has_any_modules, all_module_basenames_file):
    """Returns a step for grabbing the unstripped in-tree modules from `OUT_DIR`.

    Returns:
      A struct with these fields:

      * inputs
      * tools
      * cmd
      * outputs
      * unstripped_dir: A [File](https://bazel.build/rules/lib/File), which is a directory pointing
        to a directory containing the unstripped modules.
    """
    grab_unstripped_intree_modules_cmd = ""
    inputs = []
    tools = []
    outputs = []
    unstripped_dir = None

    if ctx.attr.collect_unstripped_modules:
        unstripped_dir = ctx.actions.declare_directory("{name}/unstripped".format(name = ctx.label.name))
        outputs.append(unstripped_dir)

        if has_any_modules:
            tools.append(ctx.executable._search_and_cp_output)
            inputs.append(all_module_basenames_file)
            grab_unstripped_intree_modules_cmd = """
                mkdir -p {unstripped_dir}
                {search_and_cp_output} --srcdir ${{OUT_DIR}} --dstdir {unstripped_dir} $(cat {all_module_basenames_file})
            """.format(
                search_and_cp_output = ctx.executable._search_and_cp_output.path,
                unstripped_dir = unstripped_dir.path,
                all_module_basenames_file = all_module_basenames_file.path,
            )

    return struct(
        inputs = inputs,
        tools = tools,
        cmd = grab_unstripped_intree_modules_cmd,
        outputs = outputs,
        unstripped_dir = unstripped_dir,
    )

def _get_check_remaining_modules_step(
        ctx,
        all_module_names,
        base_kernel_all_module_names,
        modules_staging_dir):
    """Returns a step for checking remaining '*.ko' files in `OUT_DIR`.

    Returns:
      A struct with these fields:

      * cmd
      * inputs
      * tools
      * outputs
    """

    if not ctx.attr._warn_undeclared_modules[BuildSettingInfo].value:
        return struct(
            cmd = """
            echo "Check for undeclared modules in kernel_build skipped." >&2
            """,
            inputs = [],
            tools = [],
            outputs = [],
        )

    message_type = "ERROR"
    epilog = "exit 1"
    if ctx.attr._allow_undeclared_modules[BuildSettingInfo].value:
        message_type = "WARNING"
        epilog = ""

    cmd = """
           remaining_ko_files=$({check_declared_output_list} \\
                --declared {all_module_names} {base_kernel_all_module_names} \\
                --actual $(cd {modules_staging_dir}/lib/modules/*/kernel && find . -type f -name '*.ko' | sed 's:^[.]/::'))
           if [[ ${{remaining_ko_files}} ]]; then
             echo "{message_type}: The following kernel modules are built but not copied. Add these lines to the module_outs attribute of {label}:" >&2
             for ko in ${{remaining_ko_files}}; do
               echo '    "'"${{ko}}"'",' >&2
             done
             echo "Alternatively, install buildozer and execute:" >&2
             echo "  $ buildozer 'add module_outs ${{remaining_ko_files}}' {label}" >&2
             echo "See https://github.com/bazelbuild/buildtools/blob/master/buildozer/README.md for reference" >&2
             {epilog}
           fi
    """.format(
        message_type = message_type,
        check_declared_output_list = ctx.executable._check_declared_output_list.path,
        all_module_names = " ".join(all_module_names),
        base_kernel_all_module_names = " ".join(base_kernel_all_module_names),
        modules_staging_dir = modules_staging_dir,
        label = ctx.label,
        epilog = epilog,
    )
    tools = [ctx.executable._check_declared_output_list]

    return struct(
        cmd = cmd,
        inputs = [],
        tools = tools,
        outputs = [],
    )

def _get_grab_symtypes_step(ctx):
    """Returns a step for grabbing the `*.symtypes` from `OUT_DIR`.

    Returns:
      A struct with these fields:

      * inputs
      * tools
      * outputs
      * cmd
    """
    grab_symtypes_cmd = ""
    outputs = []
    if ctx.attr.config[KernelEnvAttrInfo].kbuild_symtypes:
        symtypes_dir = ctx.actions.declare_directory("{name}/symtypes".format(name = ctx.label.name))
        outputs.append(symtypes_dir)
        grab_symtypes_cmd = """
            rsync -a --prune-empty-dirs --include '*/' --include '*.symtypes' --exclude '*' ${{OUT_DIR}}/ {symtypes_dir}/
        """.format(
            symtypes_dir = symtypes_dir.path,
        )
    return struct(
        inputs = [],
        tools = [],
        cmd = grab_symtypes_cmd,
        outputs = outputs,
    )

def _get_grab_kbuild_output_step(ctx):
    """Returns a step for grabbing the `*`files from `OUT_DIR`.

    Returns:
      A struct with fields (inputs, tools, outputs, cmd)
    """
    grab_kbuild_output_cmd = ""
    outputs = []
    if ctx.attr._preserve_kbuild_output[BuildSettingInfo].value:
        kbuild_output_target = ctx.actions.declare_directory("{name}/kbuild_output".format(name = ctx.label.name))
        outputs.append(kbuild_output_target)
        grab_kbuild_output_cmd = """
            if [[ -L ${{OUT_DIR}}/source ]]; then
                rm -f ${{OUT_DIR}}/source
            fi
            rsync -a --prune-empty-dirs --include '*/' ${{OUT_DIR}}/ {kbuild_output_target}/
        """.format(
            kbuild_output_target = kbuild_output_target.path,
        )
    return struct(
        inputs = [],
        tools = [],
        cmd = grab_kbuild_output_cmd,
        outputs = outputs,
    )

def get_grab_cmd_step(ctx, src_dir):
    """Returns a step for grabbing the `*.cmd` from `src_dir`.

    Args:
        ctx: Context from the rule.
        src_dir: Source directory.

    Returns:
        A struct with these fields:
        * inputs
        * tools
        * outputs
        * cmd
        * cmd_dir
    """
    cmd = ""
    cmd_dir = None
    outputs = []
    if ctx.attr._preserve_cmd[BuildSettingInfo].value:
        cmd_dir = ctx.actions.declare_directory("{name}/cmds".format(name = ctx.label.name))
        outputs.append(cmd_dir)
        cmd = """
            rsync -a --chmod=F+w --prune-empty-dirs --include '*/' --include '*.cmd' --exclude '*' {src_dir}/ {cmd_dir}/
            find {cmd_dir}/ -name '*.cmd' -exec sed -i'' -e 's:'"${{ROOT_DIR}}"':${{ROOT_DIR}}:g' {{}} \\+
        """.format(
            src_dir = src_dir,
            cmd_dir = cmd_dir.path,
        )
    return struct(
        inputs = [],
        tools = [],
        cmd = cmd,
        outputs = outputs,
        cmd_dir = cmd_dir,
    )

def _get_copy_module_symvers_step(ctx):
    """Returns a step for keeping a copy of Module.symvers from `OUT_DIR`.

    Returns:
      A struct with fields (inputs, tools, outputs, cmd)
    """
    copy_module_symvers_cmd = ""
    outputs = []

    if ctx.attr.keep_module_symvers:
        module_symvers_copy = ctx.actions.declare_file("{}/{}_Module.symvers".format(
            ctx.label.name,
            ctx.label.name,
        ))
        outputs.append(module_symvers_copy)
        copy_module_symvers_cmd = """
           cp -f ${{OUT_DIR}}/Module.symvers {module_symvers_copy}
        """.format(
            module_symvers_copy = module_symvers_copy.path,
        )
    return struct(
        inputs = [],
        tools = [],
        cmd = copy_module_symvers_cmd,
        outputs = outputs,
    )

def _get_modinst_step(ctx, modules_staging_dir):
    module_strip_flag = "INSTALL_MOD_STRIP="
    if ctx.attr.strip_modules:
        module_strip_flag += "1"

    base_kernel = base_kernel_utils.get_base_kernel(ctx)

    cmd = ""
    inputs = []
    tools = []

    if base_kernel:
        cmd += """
          # Check that base_kernel has the same KMI as the current kernel_build
            (
                base_release=$(cat {base_kernel_release_file})
                base_kmi=$({get_kmi_string} --keep_sublevel ${{base_release}})
                my_release=$(cat ${{OUT_DIR}}/include/config/kernel.release)
                my_kmi=$({get_kmi_string} --keep_sublevel ${{my_release}})
                if [[ "${{base_kmi}}" != "${{my_kmi}}" ]]; then
                    echo "ERROR: KMI or sublevel mismatch before running make modules_install:" >&2
                    echo "  {label}: ${{my_kmi}} (from ${{my_release}})" >&2
                    echo "  {base_kernel_label}: ${{base_kmi}} (from ${{base_release}})" >&2
                fi
            )

          # Fix up kernel.release to be the one from {base_kernel_label} before installing modules
            cp -L {base_kernel_release_file} ${{OUT_DIR}}/include/config/kernel.release
        """.format(
            get_kmi_string = ctx.executable._get_kmi_string.path,
            label = ctx.label,
            base_kernel_label = base_kernel.label,
            base_kernel_release_file = base_kernel[KernelBuildUnameInfo].kernel_release.path,
        )
        inputs.append(base_kernel[KernelBuildUnameInfo].kernel_release)
        tools.append(ctx.executable._get_kmi_string)

    cmd += """
         # Set variables and create dirs for modules
           mkdir -p {modules_staging_dir}
         # Install modules
           if grep -q -E -e "\\bmodules\\b|[.]ko\\b" <<< "{make_goals}" ; then
               if grep -q -E -e "[.]ko\\b" <<< "{make_goals}" ; then
                   # For single_module builds, we need to generate our own
                   # modules.order since `make` deletes it, but it's needed for
                   # `modules_install`. We don't really care about the module
                   # ordering here. With fw_devlink enabled in GKI, the probe
                   # ordering should be handled based on device dependencies.
                   : > ${{OUT_DIR}}/modules.order
                   for module in {make_goals}; do
                       echo ${{module}} | sed -n "s:\\([.]\\)ko\\>.*:\\1o:p"  >> ${{OUT_DIR}}/modules.order
                   done
               fi
               make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} DEPMOD=true O=${{OUT_DIR}} {module_strip_flag} INSTALL_MOD_PATH=$(realpath {modules_staging_dir}) modules_install
           else
               # Workaround as this file is required, hence just produce a placeholder.
               touch {internal_outs_under_out_dir}
           fi
    """.format(
        modules_staging_dir = modules_staging_dir,
        internal_outs_under_out_dir = " ".join(["${{OUT_DIR}}/{}".format(item) for item in _kernel_build_internal_outs]),
        module_strip_flag = module_strip_flag,
        make_goals = " ".join(ctx.attr.config[KernelEnvMakeGoalsInfo].make_goals),
    )

    if base_kernel:
        cmd += """
          # Check that `make modules_install` does not revert include/config/kernel.release
            if ! diff -q {base_kernel_release} ${{OUT_DIR}}/include/config/kernel.release; then
                echo "ERROR: make modules_install modifies include/config/kernel.release." >&2
                echo "    This is not expected; please file a bug!" >&2
                echo "    expected: $(cat {base_kernel_release})" >&2
                echo "    actual: $(cat ${{OUT_DIR}}/include/config/kernel.release)" >&2
            fi
        """.format(
            base_kernel_release = base_kernel[KernelBuildUnameInfo].kernel_release.path,
        )

    return struct(
        inputs = inputs,
        tools = tools,
        cmd = cmd,
        outputs = [],
    )

def _build_main_action(
        ctx,
        kbuild_mixed_tree_ret,
        all_output_names,
        all_module_basenames_file):
    """Adds the main action for the `kernel_build`."""
    base_kernel_all_module_names = _get_base_kernel_all_module_names(ctx)

    # Declare outputs.
    ## Declare outputs based on the *outs attributes
    all_output_files = _declare_all_output_files(ctx)

    ## Declare implicit outputs of the command
    ## This is like ctx.actions.declare_directory(ctx.label.name) without actually declaring it.
    ruledir = paths.join(
        utils.package_bin_dir(ctx),
        ctx.label.name,
    )

    if base_kernel_utils.get_base_kernel(ctx):
        # We will re-package MODULES_STAGING_ARCHIVE in _repack_module_staging_archive,
        # so use a different name.
        modules_staging_archive_self = ctx.actions.declare_file(
            "{}/modules_staging_dir_self.tar.gz".format(ctx.label.name),
        )
    else:
        modules_staging_archive_self = ctx.actions.declare_file(
            "{}/{}".format(ctx.label.name, MODULES_STAGING_ARCHIVE),
        )

    out_dir_kernel_headers_tar = ctx.actions.declare_file(
        "{name}/out-dir-kernel-headers.tar.gz".format(name = ctx.label.name),
    )

    modules_staging_dir = modules_staging_archive_self.dirname + "/staging"

    # Individual steps of the final command.
    interceptor_step = _get_interceptor_step(ctx)
    cache_dir_step = cache_dir.get_step(
        ctx = ctx,
        common_config_tags = ctx.attr.config[KernelEnvAttrInfo].common_config_tags,
        symlink_name = "build",
    )
    modinst_step = _get_modinst_step(
        ctx = ctx,
        modules_staging_dir = modules_staging_dir,
    )
    grab_intree_modules_step = _get_grab_intree_modules_step(
        ctx = ctx,
        has_any_modules = bool(all_output_names.modules),
        modules_staging_dir = modules_staging_dir,
        ruledir = ruledir,
        all_module_names = all_output_names.modules,
    )
    grab_unstripped_modules_step = _get_grab_unstripped_modules_step(
        ctx = ctx,
        has_any_modules = bool(all_output_names.modules),
        all_module_basenames_file = all_module_basenames_file,
    )
    grab_symtypes_step = _get_grab_symtypes_step(ctx)
    grab_gcno_step = get_grab_gcno_step(ctx, "${COMMON_OUT_DIR}", is_kernel_build = True)
    grab_cmd_step = get_grab_cmd_step(ctx, "${OUT_DIR}")
    compile_commands_step = compile_commands_utils.kernel_build_step(ctx)
    grab_gdb_scripts_step = kgdb.get_grab_gdb_scripts_step(ctx)
    grab_kbuild_output_step = _get_grab_kbuild_output_step(ctx)
    copy_module_symvers_step = _get_copy_module_symvers_step(ctx)
    check_remaining_modules_step = _get_check_remaining_modules_step(
        ctx = ctx,
        all_module_names = all_output_names.modules,
        base_kernel_all_module_names = base_kernel_all_module_names,
        modules_staging_dir = modules_staging_dir,
    )
    steps = (
        interceptor_step,
        cache_dir_step,
        modinst_step,
        grab_intree_modules_step,
        grab_unstripped_modules_step,
        grab_symtypes_step,
        grab_gcno_step,
        grab_cmd_step,
        compile_commands_step,
        grab_gdb_scripts_step,
        grab_kbuild_output_step,
        copy_module_symvers_step,
        check_remaining_modules_step,
    )

    # Build the command for the main action.
    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.config[KernelSerializedEnvInfo],
        restore_out_dir_cmd = cache_dir_step.cmd,
    )

    make_goals = ctx.attr.config[KernelEnvMakeGoalsInfo].make_goals
    command += """
           {kbuild_mixed_tree_cmd}
         # Actual kernel build
           if grep -q "\\bdtbs\\b" <<< "{make_goals}" ; then
               # TODO(b/359683179): if we build dtbs with the individual kernel
               # modules, then not all of the kernel modules are found in the
               # OUT_DIR. Building dtbs first, fixes this issue.
               {interceptor_command_prefix} make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} dtbs
           fi
           make_subgoals=$(echo "{make_goals}" | sed "s:\\<dtbs\\>::" || true)
           if grep -q -E -e "[.]ko\\b" <<< "${{make_subgoals}}" ; then
               # TODO(b/359681021) Follow up with the upstream maintainer to
               # see what needs to be done to drop KBUILD_MODPOST_WARN=1.
               {interceptor_command_prefix} make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} KBUILD_MODPOST_WARN=1 ${{make_subgoals}}
           else
               {interceptor_command_prefix} make -C ${{KERNEL_DIR}} ${{TOOL_ARGS}} O=${{OUT_DIR}} ${{make_subgoals}}
           fi
         # Install modules
           {modinst_cmd}
         # Archive headers in OUT_DIR
           find ${{OUT_DIR}} -name *.h -print0                          \
               | tar czf {out_dir_kernel_headers_tar}                   \
                       --absolute-names                                 \
                       --dereference                                    \
                       --transform "s,.*$OUT_DIR,,"                     \
                       --transform "s,^/,,"                             \
                       --null -T -
         # Grab outputs. If unable to find from OUT_DIR, look at KBUILD_MIXED_TREE as well.
           {search_and_cp_output} --srcdir ${{OUT_DIR}} {kbuild_mixed_tree_arg} {dtstree_arg} --dstdir {ruledir} {all_output_names_minus_modules}
         # Archive modules_staging_dir
           tar czf {modules_staging_archive_self} -C {modules_staging_dir} .
         # Grab *.symtypes
           {grab_symtypes_cmd}
         # Grab *.gcno files
           {grab_gcno_step_cmd}
         # Grab *.cmd
           {grab_cmd_cmd}
         # Grab files for compile_commands.json
           {compile_commands_step}
         # Grab GDB scripts
           {grab_gdb_scripts_cmd}
         # Grab * files
           {grab_kbuild_output_step_cmd}
         # Grab in-tree modules
           {grab_intree_modules_cmd}
         # Grab unstripped in-tree modules
           {grab_unstripped_intree_modules_cmd}
         # Make a copy of Module.symvers
           {copy_module_symvers_cmd}
           if grep -q "\\bmodules\\b" <<< "{make_goals}"; then
             # Check if there are remaining *.ko files
               {check_remaining_modules_cmd}
           fi
         # Clean up staging directories
           rm -rf {modules_staging_dir}
         # Create last_build symlink in cache_dir
           {cache_dir_post_cmd}
         """.format(
        cache_dir_post_cmd = cache_dir_step.post_cmd,
        kbuild_mixed_tree_cmd = kbuild_mixed_tree_ret.cmd,
        search_and_cp_output = ctx.executable._search_and_cp_output.path,
        kbuild_mixed_tree_arg = kbuild_mixed_tree_ret.arg,
        dtstree_arg = "--srcdir ${OUT_DIR}/${dtstree}",
        ruledir = ruledir,
        all_output_names_minus_modules = " ".join(all_output_names.non_modules),
        modinst_cmd = modinst_step.cmd,
        grab_intree_modules_cmd = grab_intree_modules_step.cmd,
        grab_unstripped_intree_modules_cmd = grab_unstripped_modules_step.cmd,
        grab_symtypes_cmd = grab_symtypes_step.cmd,
        grab_gcno_step_cmd = grab_gcno_step.cmd,
        grab_cmd_cmd = grab_cmd_step.cmd,
        compile_commands_step = compile_commands_step.cmd,
        grab_gdb_scripts_cmd = grab_gdb_scripts_step.cmd,
        grab_kbuild_output_step_cmd = grab_kbuild_output_step.cmd,
        check_remaining_modules_cmd = check_remaining_modules_step.cmd,
        modules_staging_dir = modules_staging_dir,
        modules_staging_archive_self = modules_staging_archive_self.path,
        out_dir_kernel_headers_tar = out_dir_kernel_headers_tar.path,
        interceptor_command_prefix = interceptor_step.command_prefix,
        label = ctx.label,
        make_goals = " ".join(make_goals),
        copy_module_symvers_cmd = copy_module_symvers_step.cmd,
    )

    # all inputs that |command| needs
    transitive_inputs = [target.files for target in ctx.attr.srcs]
    transitive_inputs += [target.files for target in ctx.attr.deps]
    transitive_inputs.append(
        ctx.attr.config[KernelSerializedEnvInfo].inputs,
    )
    inputs = []
    inputs += kbuild_mixed_tree_ret.outputs
    for step in steps:
        inputs += step.inputs

    # All tools that |command| needs
    tools = [
        ctx.executable._search_and_cp_output,
    ]
    transitive_tools = [
        ctx.attr.config[KernelSerializedEnvInfo].tools,
    ]
    for step in steps:
        tools += step.tools

    # all outputs that |command| generates
    command_outputs = [
        modules_staging_archive_self,
        out_dir_kernel_headers_tar,
    ]
    for d in all_output_files.values():
        command_outputs += d.values()
    for step in steps:
        command_outputs += step.outputs

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "KernelBuild",
        inputs = depset(_uniq(inputs), transitive = transitive_inputs),
        outputs = command_outputs,
        tools = depset(_uniq(tools), transitive = transitive_tools),
        progress_message = "Building kernel {}".format(_progress_message_suffix(ctx)),
        command = command,
        execution_requirements = kernel_utils.local_exec_requirements(ctx),
    )

    return struct(
        all_output_files = all_output_files,
        out_dir_kernel_headers_tar = out_dir_kernel_headers_tar,
        interceptor_output = interceptor_step.output_file,
        modules_staging_archive_self = modules_staging_archive_self,
        unstripped_dir = grab_unstripped_modules_step.unstripped_dir,
        ruledir = ruledir,
        cmd_dir = grab_cmd_step.cmd_dir,
        compile_commands_with_vars = compile_commands_step.compile_commands_with_vars,
        compile_commands_out_dir = compile_commands_step.compile_commands_out_dir,
        gcno_outputs = grab_gcno_step.outputs,
        gcno_mapping = grab_gcno_step.gcno_mapping,
        gcno_dir = grab_gcno_step.gcno_dir,
        module_symvers_outputs = copy_module_symvers_step.outputs,
    )

def create_serialized_env_info(
        ctx,
        setup_script_name,
        pre_info,
        outputs,
        fake_system_map,
        extra_restore_outputs_cmd,
        extra_inputs):
    """Creates an KernelSerializedEnvInfo.

    Args:
        ctx: ctx,
        setup_script_name: name of the setup script
        pre_info: KernelSerializedEnvInfo
        outputs: dictionary where
            keys are `File`, and values are the relative paths under $OUT_DIR as the
            destination
        fake_system_map: Whether to create a fake `$OUT_DIR/System.map`
        extra_restore_outputs_cmd: Extra CMD to restore outputs
        extra_inputs: a depset attached to `inputs` of returned object

    Returns:
        A KernelSerializedEnvInfo that runs pre_info, then restore outputs given the list of
        outputs and cmd."""

    restore_outputs_cmd = \
        _get_serialized_env_info_setup_restore_outputs_command(
            outputs = outputs,
            fake_system_map = fake_system_map,
        )
    restore_outputs_cmd += extra_restore_outputs_cmd

    setup_script = ctx.actions.declare_file(setup_script_name)
    setup_script_cmd = """
        . {pre_setup_script}
        {restore_outputs_cmd}
    """.format(
        pre_setup_script = pre_info.setup_script.path,
        restore_outputs_cmd = restore_outputs_cmd,
    )
    ctx.actions.write(
        output = setup_script,
        content = setup_script_cmd,
    )
    return KernelSerializedEnvInfo(
        setup_script = setup_script,
        inputs = depset(
            [setup_script],
            transitive = [
                pre_info.inputs,
                extra_inputs,
                depset(outputs.keys()),
            ],
        ),
        tools = pre_info.tools,
    )

def _get_serialized_env_info_setup_restore_outputs_command(outputs, fake_system_map):
    """Returns the `restore_outputs` command for the environment to build kernel_module.

    Args:
        outputs: dictionary where
            keys are `File`, and values are the relative paths under $OUT_DIR as the
            destinastion
        fake_system_map: Whether to create a fake `$OUT_DIR/System.map`
    Returns:
        the `restore_outputs` command for the environment to build kernel_module.
    """

    cmd = ""
    if outputs:
        cmd += """
            # Restore kernel build outputs
        """
    for dep, relpath in outputs.items():
        cmd += """
            mkdir -p $(dirname ${{OUT_DIR}}/{relpath})
            rsync -aL {dep} ${{OUT_DIR}}/{relpath}
        """.format(
            dep = dep.path,
            relpath = relpath,
        )

    # If System.map does not already exist, create a fake System.map because
    # `make modules` does not need it. For kernel_module(),
    # make modules_install needs it, but we aren't running depmod in
    # kernel_module, so a fake one is good enough.
    if fake_system_map:
        cmd += """
            touch ${OUT_DIR}/System.map
        """

    return cmd

def _create_infos(
        ctx,
        kbuild_mixed_tree_ret,
        all_module_names,
        main_action_ret,
        modules_staging_archive,
        kmi_strict_mode_out,
        kmi_symbol_list_violations_check_out,
        module_scripts_archive,
        module_srcs):
    """Creates and returns a list of provided infos that the `kernel_build` target should return.

    Args:
        ctx: ctx
        kbuild_mixed_tree_ret: from `_create_kbuild_mixed_tree`
        all_module_names: `module_outs` + `module_implicit_outs`
        main_action_ret: from `_build_main_action`
        modules_staging_archive: from `_repack_modules_staging_archive`
        kmi_strict_mode_out: from `_kmi_symbol_list_strict_mode`
        kmi_symbol_list_violations_check_out: from `_kmi_symbol_list_violations_check`
        module_srcs: from `kernel_utils.filter_module_srcs`
        module_scripts_archive: from `_create_module_scripts_archive`
    """

    base_kernel = base_kernel_utils.get_base_kernel(ctx)

    all_output_files = main_action_ret.all_output_files

    # outs and internal_outs are needed. implicit_outs are needed to
    # build GKI's system_dlkm image to sign modules. Modules are not needed.
    serialized_env_info_dependencies = list(all_output_files["outs"].values())
    serialized_env_info_dependencies += all_output_files["internal_outs"].values()
    serialized_env_info_dependencies += all_output_files["implicit_outs"].values()

    serialized_env_info = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_setup.sh".format(name = ctx.attr.name),
        pre_info = ctx.attr.config[KernelSerializedEnvInfo],
        outputs = {
            dep: paths.relativize(dep.path, main_action_ret.ruledir)
            for dep in serialized_env_info_dependencies
        },
        fake_system_map = False,
        extra_restore_outputs_cmd = kbuild_mixed_tree_ret.cmd,
        extra_inputs = depset(kbuild_mixed_tree_ret.outputs),
    )

    orig_env_info = ctx.attr.config[KernelBuildOriginalEnvInfo]

    kernel_build_info = KernelBuildInfo(
        out_dir_kernel_headers_tar = main_action_ret.out_dir_kernel_headers_tar,
        outs = depset(all_output_files["outs"].values()),
        base_kernel_files = kbuild_mixed_tree_ret.base_kernel_files,
        interceptor_output = main_action_ret.interceptor_output,
        compile_commands_with_vars = main_action_ret.compile_commands_with_vars,
        compile_commands_out_dir = main_action_ret.compile_commands_out_dir,
    )

    kernel_build_uname_info = KernelBuildUnameInfo(
        kernel_release = all_output_files["internal_outs"]["include/config/kernel.release"],
    )

    # For kernel_module()
    ext_mod_serialized_env_info_deps = all_output_files["internal_outs"].values()
    mod_min_env = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_mod_min_setup.sh".format(name = ctx.attr.name),
        pre_info = ctx.attr.modules_prepare[KernelSerializedEnvInfo],
        outputs = {
            dep: paths.relativize(dep.path, main_action_ret.ruledir)
            for dep in ext_mod_serialized_env_info_deps
        },
        fake_system_map = True,
        extra_restore_outputs_cmd = """
            (
                {kbuild_mixed_tree_cmd}
              # Stitch together the GKI symbols and device module symbols
              # together for proper linking during the modpost step. This will
              # also help detect CRC differences between the GKI and device
              # builds if the same symbol is defined twice with different CRC
              # values.
                if [[ -f "${{KBUILD_MIXED_TREE}}/Module.symvers" ]]; then
                  cp ${{KBUILD_MIXED_TREE}}/Module.symvers ${{OUT_DIR}}/Module.symvers.tmp
                  grep -v -e "\\bvmlinux\\b" ${{OUT_DIR}}/Module.symvers >> ${{OUT_DIR}}/Module.symvers.tmp
                  cat ${{OUT_DIR}}/Module.symvers.tmp | sort -u > ${{OUT_DIR}}/Module.symvers
                  rm -f ${{OUT_DIR}}/Module.symvers.tmp
                fi
            )
        """.format(
            kbuild_mixed_tree_cmd = kbuild_mixed_tree_ret.cmd,
        ),
        extra_inputs = depset(kbuild_mixed_tree_ret.outputs, transitive = [module_srcs.module_scripts]),
    )

    # External modules do not need implicit_outs because they are unsigned.
    ext_mod_full_serialized_env_info_dependencies = list(all_output_files["outs"].values())
    ext_mod_full_serialized_env_info_dependencies += all_output_files["internal_outs"].values()

    # For kernel_module() that require all kernel_build outputs and kernel_modules_install()
    mod_full_env = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_mod_full_setup.sh".format(name = ctx.attr.name),
        pre_info = ctx.attr.modules_prepare[KernelSerializedEnvInfo],
        outputs = {
            dep: paths.relativize(dep.path, main_action_ret.ruledir)
            for dep in ext_mod_full_serialized_env_info_dependencies
        },
        fake_system_map = False,
        extra_restore_outputs_cmd = kbuild_mixed_tree_ret.cmd,
        extra_inputs = depset(
            kbuild_mixed_tree_ret.outputs,
            transitive = [module_srcs.module_scripts],
        ),
    )

    # For ddk_config()
    ddk_config_env = create_serialized_env_info(
        ctx = ctx,
        setup_script_name = "{name}/{name}_ddk_config_setup.sh".format(name = ctx.attr.name),
        pre_info = ctx.attr.config[KernelSerializedEnvInfo],
        outputs = {},
        fake_system_map = False,
        extra_restore_outputs_cmd = "",
        extra_inputs = depset(transitive = [
            module_srcs.module_scripts,
            module_srcs.module_kconfig,
        ]),
    )

    ddk_module_defconfig_fragments = depset(transitive = [
        target.files
        for target in ctx.attr.ddk_module_defconfig_fragments
    ])

    kernel_build_module_info = KernelBuildExtModuleInfo(
        modules_staging_archive = modules_staging_archive,
        module_hdrs = module_srcs.module_hdrs,
        ddk_config_env = ddk_config_env,
        mod_min_env = mod_min_env,
        mod_full_env = mod_full_env,
        modinst_env = mod_full_env,
        collect_unstripped_modules = ctx.attr.collect_unstripped_modules,
        strip_modules = ctx.attr.strip_modules,
        ddk_module_defconfig_fragments = ddk_module_defconfig_fragments,
    )

    kernel_uapi_depsets = []
    if base_kernel:
        kernel_uapi_depsets.append(base_kernel[KernelBuildUapiInfo].kernel_uapi_headers)
    kernel_uapi_depsets.append(ctx.attr.kernel_uapi_headers.files)
    kernel_uapi_headers_depset = depset(transitive = kernel_uapi_depsets, order = "postorder")
    kernel_build_uapi_info = KernelBuildUapiInfo(
        kernel_uapi_headers = kernel_uapi_headers_depset,
    )

    if ctx.files.combined_abi_symbollist:
        if len(ctx.files.combined_abi_symbollist) > 1:
            fail("{}: combined_abi_symbollist must only provide at most one file".format(ctx.label))
        combined_abi_symbollist = ctx.files.combined_abi_symbollist[0]
    else:
        combined_abi_symbollist = None

    kernel_build_abi_info = KernelBuildAbiInfo(
        trim_nonlisted_kmi = trim_nonlisted_kmi_utils.get_value(ctx),
        combined_abi_symbollist = combined_abi_symbollist,
        modules_staging_archive = modules_staging_archive,
        base_modules_staging_archive = base_kernel_utils.get_base_modules_staging_archive(ctx),
        src_protected_exports_list = ctx.file.src_protected_exports_list,
        src_protected_modules_list = ctx.file.src_protected_modules_list,
        src_kmi_symbol_list = ctx.file.src_kmi_symbol_list,
        kmi_strict_mode_out = kmi_strict_mode_out,
    )

    # Device modules takes precedence over base_kernel (GKI) modules.
    unstripped_modules_depsets = []
    if main_action_ret.unstripped_dir:
        unstripped_modules_depsets.append(depset([main_action_ret.unstripped_dir]))
    if base_kernel:
        unstripped_modules_depsets.append(base_kernel[KernelUnstrippedModulesInfo].directories)
    kernel_unstripped_modules_info = KernelUnstrippedModulesInfo(
        directories = depset(transitive = unstripped_modules_depsets, order = "postorder"),
    )

    in_tree_modules_info = KernelBuildInTreeModulesInfo(
        all_module_names = all_module_names,
    )

    images_info = KernelImagesInfo(
        base_kernel_label = base_kernel.label if base_kernel else None,
        outs = depset(all_output_files["outs"].values()),
        base_kernel_files = kbuild_mixed_tree_ret.base_kernel_files,
    )

    gcov_info = GcovInfo(
        gcno_mapping = main_action_ret.gcno_mapping,
        gcno_dir = main_action_ret.gcno_dir,
    )

    output_group_kwargs = {}
    for d in all_output_files.values():
        output_group_kwargs.update({name: depset([file]) for name, file in d.items()})

    # TODO(b/291918087): Drop after common_kernels no longer use kernel_filegroup.
    #   These files should already be in kernel_filegroup_declaration.
    output_group_kwargs["modules_staging_archive"] = depset([modules_staging_archive])
    output_group_info = OutputGroupInfo(**output_group_kwargs)

    kbuild_mixed_tree_files = all_output_files["outs"].values() + all_output_files["module_outs"].values()
    kbuild_mixed_tree_files.append(all_output_files["internal_outs"]["Module.symvers"])
    kbuild_mixed_tree_info = KernelBuildMixedTreeInfo(
        files = depset(kbuild_mixed_tree_files),
    )

    cmds_info = KernelCmdsInfo(
        srcs = depset([target.files for target in ctx.attr.srcs]),
        directories = depset([main_action_ret.cmd_dir]),
    )

    modules_prepare_archive = utils.find_file(
        _MODULES_PREPARE_ARCHIVE,
        ctx.files.modules_prepare,
        what = ctx.label,
        required = True,
    )

    filegroup_decl_info = KernelBuildFilegroupDeclInfo(
        filegroup_srcs = depset(all_output_files["outs"].values() +
                                all_output_files["module_outs"].values()),
        all_module_names = all_module_names,
        modules_staging_archive = modules_staging_archive,
        toolchain_version = ctx.attr.config[KernelToolchainInfo].toolchain_version,
        kernel_release = all_output_files["internal_outs"]["include/config/kernel.release"],
        modules_prepare_archive = modules_prepare_archive,
        collect_unstripped_modules = ctx.attr.collect_unstripped_modules,
        strip_modules = ctx.attr.strip_modules,
        src_protected_modules_list = ctx.file.src_protected_modules_list,
        ddk_module_defconfig_fragments = ddk_module_defconfig_fragments,
        kernel_uapi_headers = kernel_uapi_headers_depset,
        arch = ctx.attr.arch,
        env_setup_script = ctx.attr.config[KernelConfigInfo].env_setup_script,
        config_out_dir = ctx.file.config,
        outs = depset(all_output_files["outs"].values()),
        internal_outs = depset(all_output_files["internal_outs"].values()),
        ruledir = main_action_ret.ruledir,
        module_env_archive = module_scripts_archive,
        has_base_kernel = base_kernel_utils.get_base_kernel(ctx) != None,
    )

    default_info_files = all_output_files["outs"].values() + all_output_files["module_outs"].values()
    if kmi_strict_mode_out:
        default_info_files.append(kmi_strict_mode_out)
    default_info_files.extend(main_action_ret.module_symvers_outputs)
    default_info_files.extend(main_action_ret.gcno_outputs)
    if kmi_symbol_list_violations_check_out:
        default_info_files.append(kmi_symbol_list_violations_check_out)
    default_info = DefaultInfo(
        files = depset(default_info_files),
        # For kernel_build_test
        runfiles = ctx.runfiles(files = default_info_files),
    )

    return [
        cmds_info,
        serialized_env_info,
        orig_env_info,
        kbuild_mixed_tree_info,
        kernel_build_info,
        kernel_build_module_info,
        kernel_build_uapi_info,
        kernel_build_uname_info,
        kernel_build_abi_info,
        kernel_unstripped_modules_info,
        in_tree_modules_info,
        images_info,
        gcov_info,
        filegroup_decl_info,
        ctx.attr.config[KernelEnvAttrInfo],
        ctx.attr.config[KernelToolchainInfo],
        output_group_info,
        default_info,
    ]

def _kernel_build_impl(ctx):
    kbuild_mixed_tree_ret = _create_kbuild_mixed_tree(ctx)
    _kernel_build_check_toolchain(ctx)

    all_output_names = _split_out_attrs(ctx)

    # A file containing the basenames of the modules
    all_module_basenames_file = _write_module_names_to_file(
        ctx,
        "all_module_basenames.txt",
        [paths.basename(filename) for filename in all_output_names.modules],
    )

    main_action_ret = _build_main_action(
        ctx = ctx,
        kbuild_mixed_tree_ret = kbuild_mixed_tree_ret,
        all_output_names = all_output_names,
        all_module_basenames_file = all_module_basenames_file,
    )

    modules_staging_archive = _repack_modules_staging_archive(
        ctx = ctx,
        modules_staging_archive_self = main_action_ret.modules_staging_archive_self,
        all_module_basenames_file = all_module_basenames_file,
    )

    kmi_strict_mode_out = _kmi_symbol_list_strict_mode(
        ctx = ctx,
        all_output_files = main_action_ret.all_output_files,
        all_module_names = all_output_names.modules,
    )

    kmi_symbol_list_violations_check_out = _kmi_symbol_list_violations_check(ctx, modules_staging_archive)

    module_srcs = kernel_utils.filter_module_srcs(ctx.files.srcs)

    module_scripts_archive = _create_module_scripts_archive(
        ctx = ctx,
        module_srcs = module_srcs,
    )

    infos = _create_infos(
        ctx = ctx,
        kbuild_mixed_tree_ret = kbuild_mixed_tree_ret,
        all_module_names = all_output_names.modules,
        main_action_ret = main_action_ret,
        modules_staging_archive = modules_staging_archive,
        kmi_strict_mode_out = kmi_strict_mode_out,
        kmi_symbol_list_violations_check_out = kmi_symbol_list_violations_check_out,
        module_scripts_archive = module_scripts_archive,
        module_srcs = module_srcs,
    )

    return infos

def _kernel_build_additional_attrs():
    return dicts.add(
        kernel_config_settings.of_kernel_build(),
        base_kernel_utils.non_config_attrs(),
        cache_dir.attrs(),
    )

# Sync with kleaf/bazel.py
_kernel_build = rule(
    implementation = _kernel_build_impl,
    doc = "Defines a kernel build target.",
    attrs = {
        "config": attr.label(
            mandatory = True,
            providers = [
                KernelSerializedEnvInfo,
                KernelEnvAttrInfo,
                KernelEnvMakeGoalsInfo,
                KernelToolchainInfo,
            ],
            doc = "the kernel_config target",
            allow_single_file = True,
        ),
        "keep_module_symvers": attr.bool(
            doc = "If true, a copy of `Module.symvers` is kept, with the name `{name}_Module.symvers`",
        ),
        "srcs": attr.label_list(mandatory = True, doc = "kernel sources", allow_files = True),
        "outs": attr.string_list(),
        "module_outs": attr.string_list(doc = "output *.ko files"),
        "internal_outs": attr.string_list(doc = "Like `outs`, but not in dist"),
        "module_implicit_outs": attr.string_list(doc = "Like `module_outs`, but not in dist"),
        "implicit_outs": attr.string_list(doc = "Like `outs`, but not in dist"),
        "_check_declared_output_list": attr.label(
            default = Label("//build/kernel/kleaf:check_declared_output_list"),
            cfg = "exec",
            executable = True,
        ),
        "_search_and_cp_output": attr.label(
            default = Label("//build/kernel/kleaf:search_and_cp_output"),
            cfg = "exec",
            executable = True,
            doc = "label referring to the script to process outputs",
        ),
        "deps": attr.label_list(
            allow_files = True,
        ),
        "kmi_symbol_list_strict_mode": attr.bool(),
        "raw_kmi_symbol_list": attr.label(
            doc = "Label to abi_symbollist.raw. Must be 0 or 1 file.",
            allow_files = True,
        ),
        "collect_unstripped_modules": attr.bool(),
        "enable_interceptor": attr.bool(),
        "_verify_ksymtab": attr.label(
            default = "//build/kernel:abi_verify_ksymtab",
            executable = True,
            cfg = "exec",
        ),
        "_check_symbol_protection": attr.label(
            default = "//build/kernel:check_buildtime_symbol_protection",
            executable = True,
            cfg = "exec",
        ),
        "_get_kmi_string": attr.label(
            default = "//build/kernel/kleaf/impl:get_kmi_string",
            executable = True,
            cfg = "exec",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_allow_undeclared_modules": attr.label(default = "//build/kernel/kleaf:allow_undeclared_modules"),
        "_warn_undeclared_modules": attr.label(default = "//build/kernel/kleaf:warn_undeclared_modules"),
        "_preserve_cmd": attr.label(default = "//build/kernel/kleaf/impl:preserve_cmd"),
        "_kmi_symbol_list_violations_check": attr.label(default = "//build/kernel/kleaf:kmi_symbol_list_violations_check"),
        # Though these rules are unrelated to the `_kernel_build` rule, they are added as fake
        # dependencies so KernelBuildExtModuleInfo and KernelBuildUapiInfo works.
        # There are no real dependencies. Bazel does not build these targets before building the
        # `_kernel_build` target.
        "modules_prepare": attr.label(providers = [KernelSerializedEnvInfo]),
        "kernel_uapi_headers": attr.label(),
        "combined_abi_symbollist": attr.label(
            doc = """The **combined** `abi_symbollist` file, consist of `kmi_symbol_list` and
                `additional_kmi_symbol_lists`. Must be 0 or 1 file.""",
            allow_files = True,
        ),
        "strip_modules": attr.bool(default = False, doc = "if set, debug information won't be kept for distributed modules.  Note, modules will still be stripped when copied into the ramdisk."),
        "src_protected_exports_list": attr.label(allow_single_file = True),
        "src_protected_modules_list": attr.label(allow_single_file = True),
        "src_kmi_symbol_list": attr.label(allow_single_file = True),
        "pack_module_env": attr.bool(default = False, doc = "Create `<name>_module_scripts.tar.gz`."),
        "sanitizers": attr.string_list(
            allow_empty = False,
            default = ["default"],
        ),
        "ddk_module_defconfig_fragments": attr.label_list(
            doc = "Additional defconfig fragments for dependant DDK modules.",
            allow_empty = True,
            allow_files = True,
        ),
        "arch": attr.string(),
    } | _kernel_build_additional_attrs() | gcov_attrs(),
    toolchains = [hermetic_toolchain.type],
)

def _kernel_build_check_toolchain(ctx):
    """Checks toolchain_version is the same as base_kernel at analysis phase."""

    base_kernel = base_kernel_utils.get_base_kernel(ctx)
    if not base_kernel:
        return

    this_toolchain = ctx.attr.config[KernelToolchainInfo].toolchain_version
    base_toolchain = base_kernel[KernelToolchainInfo].toolchain_version

    if this_toolchain != base_toolchain:
        fail("""{this_label}:

ERROR: `toolchain_version` is "{this_toolchain}" for "{this_label}", but
       `toolchain_version` is "{base_toolchain}" for "{base_kernel}" (`base_kernel`).
       They must use the same `toolchain_version`.

       Fix by setting `toolchain_version` of "{this_label}"
       to be the one used by "{base_kernel}".
       If "{base_kernel}" does not set `toolchain_version` explicitly, do not set
       `toolchain_version` for "{this_label}" either.
""".format(
            this_label = ctx.label,
            this_toolchain = this_toolchain,
            base_kernel = base_kernel.label,
            base_toolchain = base_toolchain,
        ))

def _kmi_symbol_list_strict_mode(ctx, all_output_files, all_module_names):
    """Run for `KMI_SYMBOL_LIST_STRICT_MODE`.
    """
    if not ctx.attr._use_kmi_symbol_list_strict_mode[BuildSettingInfo].value:
        # buildifier: disable=print
        print("\nWARNING: {this_label}: Attribute kmi_symbol_list_strict_mode\
              IGNORED because --nokmi_symbol_list_strict_mode is set!".format(
            this_label = ctx.label,
        ))
        return None

    if _skip_build_checks(ctx, what = "Attribute kmi_symbol_list_strict_mode"):
        return None

    if not ctx.attr.kmi_symbol_list_strict_mode:
        return None
    if not ctx.files.raw_kmi_symbol_list:
        fail("{}: kmi_symbol_list_strict_mode requires kmi_symbol_list or additional_kmi_symbol_lists.")
    if len(ctx.files.raw_kmi_symbol_list) > 1:
        fail("{}: raw_kmi_symbol_list must only provide at most one file".format(ctx.label))

    vmlinux = all_output_files["outs"].get("vmlinux")
    if not vmlinux:
        fail("{}: with kmi_symbol_list_strict_mode, outs does not contain vmlinux")
    module_symvers = all_output_files["internal_outs"].get("Module.symvers")
    if not module_symvers:
        fail("{}: with kmi_symbol_list_strict_mode, outs does not contain module_symvers")

    inputs = [
        module_symvers,
    ]
    inputs += ctx.files.raw_kmi_symbol_list  # This is 0 or 1 file
    transitive_inputs = [ctx.attr.config[KernelSerializedEnvInfo].inputs]
    tools = [ctx.executable._verify_ksymtab]
    transitive_tools = [ctx.attr.config[KernelSerializedEnvInfo].tools]

    out = ctx.actions.declare_file("{}_kmi_strict_out/kmi_symbol_list_strict_mode_checked".format(ctx.attr.name))

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.config[KernelSerializedEnvInfo],
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )
    command += """
        {verify_ksymtab} \\
            --symvers-file {module_symvers} \\
            --raw-kmi-symbol-list {raw_kmi_symbol_list} \\
            --objects {vmlinux_base} {all_module_names}
        touch {out}
    """.format(
        vmlinux_base = vmlinux.basename,  # A fancy way of saying "vmlinux"
        all_module_names = " ".join([m.removesuffix(".ko") for m in all_module_names]),
        verify_ksymtab = ctx.executable._verify_ksymtab.path,
        module_symvers = module_symvers.path,
        raw_kmi_symbol_list = ctx.files.raw_kmi_symbol_list[0].path,
        out = out.path,
    )
    debug.print_scripts(ctx, command, what = "kmi_symbol_list_strict_mode")
    ctx.actions.run_shell(
        mnemonic = "KernelBuildKmiSymbolListStrictMode",
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = depset(tools, transitive = transitive_tools),
        outputs = [out],
        command = command,
        progress_message = "Checking for kmi_symbol_list_strict_mode {}".format(_progress_message_suffix(ctx)),
    )
    return out

def _kmi_symbol_list_violations_check(ctx, modules_staging_archive):
    """Checks GKI modules' symbol violations at build time.

    Args:
        ctx: ctx
        modules_staging_archive: The `modules_staging_archive` from `make`
            in `_build_main_action`.

    Returns:
        Marker file `kmi_symbol_list_violations_checked` indicating the check
        has been performed.
    """

    if not ctx.attr._kmi_symbol_list_violations_check[BuildSettingInfo].value:
        return None

    if not ctx.files.raw_kmi_symbol_list:
        return None
    if len(ctx.files.raw_kmi_symbol_list) > 1:
        fail("{}: raw_kmi_symbol_list must only provide at most one file".format(ctx.label))

    if _skip_build_checks(ctx, what = "Symbol list violations check"):
        return None

    # Skip for sanitizer build as they are not valid GKI releasae configurations.
    # Downstreams are expect to build kernel+modules+vendor modules locally
    # and can disable the runtime symbol protection with CONFIG_SIG_PROTECT=n
    # if required.
    if ctx.attr.sanitizers[0] != "default":
        return None

    inputs = [
        modules_staging_archive,
    ]
    inputs += ctx.files.raw_kmi_symbol_list  # This is 0 or 1 file
    tools = [ctx.executable._check_symbol_protection]

    # llvm-nm is needed to extract symbols.
    # Use kernel_env as _hermetic_tools is not enough.
    transitive_inputs = [ctx.attr.config[KernelBuildOriginalEnvInfo].env_info.inputs]
    transitive_tools = [ctx.attr.config[KernelBuildOriginalEnvInfo].env_info.tools]

    out = ctx.actions.declare_file(
        "{}_kmi_symbol_list_violations/{}_kmi_symbol_list_violations_checked".format(
            ctx.attr.name,
            ctx.attr.name,
        ),
    )
    intermediates_dir = utils.intermediates_dir(ctx)

    command = ctx.attr.config[KernelBuildOriginalEnvInfo].env_info.setup
    command += """
        mkdir -p {intermediates_dir}
        tar xf {modules_staging_archive} -C {intermediates_dir}
        {check_symbol_protection} \\
            --abi-symbol-list {raw_kmi_symbol_list} \\
            {intermediates_dir}
        rm -rf {intermediates_dir}
        touch {out}
    """.format(
        check_symbol_protection = ctx.executable._check_symbol_protection.path,
        intermediates_dir = intermediates_dir,
        modules_staging_archive = modules_staging_archive.path,
        out = out.path,
        raw_kmi_symbol_list = ctx.files.raw_kmi_symbol_list[0].path,
    )

    debug.print_scripts(ctx, command, what = "kmi_symbol_list_violations_check")

    ctx.actions.run_shell(
        mnemonic = "KernelBuildCheckSymbolViolations",
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = depset(tools, transitive = transitive_tools),
        outputs = [out],
        command = command,
        progress_message = "Checking for kmi_symbol_list_violations {}".format(_progress_message_suffix(ctx)),
    )

    return out

def _repack_modules_staging_archive(
        ctx,
        modules_staging_archive_self,
        all_module_basenames_file):
    """Repackages `modules_staging_archive` to contain kernel modules from `base_kernel` as well.

    Args:
        ctx: ctx
        modules_staging_archive_self: The `modules_staging_archive` from `make`
            in `_build_main_action`.
        all_module_basenames_file: Complete list of base names.
    """
    hermetic_tools = hermetic_toolchain.get(ctx)
    if not base_kernel_utils.get_base_kernel(ctx):
        # No need to repack.
        if not modules_staging_archive_self.basename == MODULES_STAGING_ARCHIVE:
            fail("\nFATAL: {}: modules_staging_archive_self.basename == {}, but not {}".format(
                ctx.label,
                modules_staging_archive_self.basename,
                MODULES_STAGING_ARCHIVE,
            ))
        return modules_staging_archive_self

    modules_staging_archive = ctx.actions.declare_file(
        "{}/{}".format(ctx.label.name, MODULES_STAGING_ARCHIVE),
    )

    # Re-package module_staging_dir to also include the one from base_kernel.
    # Pick ko files only from base_kernel, while keeping all depmod files from self.
    modules_staging_dir = modules_staging_archive.dirname + "/staging"
    cmd = hermetic_tools.setup + """
        mkdir -p {modules_staging_dir}
        tar xf {self_archive} -C {modules_staging_dir}

        # Filter out device-customized modules that has the same name as GKI modules
        base_modules=$(tar tf {base_archive} | grep '[.]ko$' || true)
        for module in $(cat {all_module_basenames_file}); do
          base_modules=$(echo "${{base_modules}}" | grep -v "${{module}}"'$' || true)
        done

        if [[ -n "${{base_modules}}" ]]; then
            tar xf {base_archive} -C {modules_staging_dir} ${{base_modules}}
        fi
        tar czf {out_archive} -C  {modules_staging_dir} .
        rm -rf {modules_staging_dir}
    """.format(
        modules_staging_dir = modules_staging_dir,
        self_archive = modules_staging_archive_self.path,
        base_archive = base_kernel_utils.get_base_kernel(ctx)[KernelBuildExtModuleInfo].modules_staging_archive.path,
        out_archive = modules_staging_archive.path,
        all_module_basenames_file = all_module_basenames_file.path,
    )
    debug.print_scripts(ctx, cmd, what = "repackage_modules_staging_archive")
    ctx.actions.run_shell(
        mnemonic = "KernelBuildModuleStagingArchive",
        inputs = [
            modules_staging_archive_self,
            base_kernel_utils.get_base_kernel(ctx)[KernelBuildExtModuleInfo].modules_staging_archive,
            all_module_basenames_file,
        ],
        outputs = [modules_staging_archive],
        tools = hermetic_tools.deps,
        progress_message = "Repackaging modules_staging_archive {}".format(_progress_message_suffix(ctx)),
        command = cmd,
    )
    return modules_staging_archive

# TODO(b/291918087): Merge into filegroup_decl.tar.gz to flatten the archive.
def _create_module_scripts_archive(
        ctx,
        module_srcs):
    """Create `{name}_module_scripts.tar.gz`

    Args:
        ctx: ctx
        module_srcs: from `kernel_utils.filter_module_srcs`
    """
    if not ctx.attr.pack_module_env:
        return None

    hermetic_tools = hermetic_toolchain.get(ctx)
    out = ctx.actions.declare_file("{name}/{name}{suffix}".format(
        name = ctx.label.name,
        suffix = MODULE_ENV_ARCHIVE_SUFFIX,
    ))

    tar_srcs = depset(transitive = [
        module_srcs.module_scripts,
        module_srcs.module_kconfig,
    ])

    cmd = hermetic_tools.setup + """
        # Create archive of module_scripts/module_kconfig
        tar cf {out} --dereference -T "$@"
    """.format(
        out = out.path,
    )

    args = ctx.actions.args()
    args.use_param_file("%s", use_always = True)

    # Uniquify for shorter script, and due to https://github.com/landley/toybox/issues/457
    args.add_all(tar_srcs, uniquify = True)

    ctx.actions.run_shell(
        mnemonic = "KernelBuildModuleScriptsArchive",
        inputs = depset(transitive = [
            tar_srcs,
        ]),
        outputs = [out],
        tools = hermetic_tools.deps,
        command = cmd,
        arguments = [args],
        progress_message = "Archiving scripts/kconfig for ext module {}".format(_progress_message_suffix(ctx)),
    )
    return out
